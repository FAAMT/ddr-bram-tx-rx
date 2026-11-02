// The slide as a .pptx: every SVG element becomes a native PowerPoint shape, so the deck can be edited rather
// than just shown. The browser has already resolved the CSS, so each element's computed style is the source of
// truth and nothing here repeats the stylesheet.
//
// Scale: one SVG unit is one CSS pixel, 1/96 in, the same as the PDF page. Type keeps the slide schema's
// 1 px = 0.75 pt, so 24-unit text is 18 pt.
//
// pptxgenjs loads only when someone asks for a .pptx; viewing the page fetches nothing extra.
(function () {
  var LIB = 'https://cdn.jsdelivr.net/npm/pptxgenjs@3.12.0/dist/pptxgen.bundle.js';
  var SRI = 'sha384-Cck14aA9cifjYolcnjebXRfWGkz5ltHMBiG4px/j8GS+xQcb7OhNQWZYyWjQ+UwQ';
  var IN = 1 / 96, PT = 0.75;
  var SANS = 'Franklin Gothic Book', MONO = 'Consolas';
  var loading = null;

  function load() {
    if (window.PptxGenJS) return Promise.resolve();
    if (!loading) loading = new Promise(function (ok, fail) {
      var s = document.createElement('script');
      s.src = LIB; s.integrity = SRI; s.crossOrigin = 'anonymous';
      s.onload = ok;
      s.onerror = function () { loading = null; fail(new Error('could not load pptxgenjs')); };
      document.head.appendChild(s);
    });
    return loading;
  }

  function hex(c) {
    var m = c && c.match(/rgba?\(([\d.]+),\s*([\d.]+),\s*([\d.]+)(?:,\s*([\d.]+))?/);
    if (!m || m[4] === '0') return null;
    return [m[1], m[2], m[3]].map(function (v) { return (+v | 0).toString(16).padStart(2, '0'); }).join('').toUpperCase();
  }

  function fillOf(cs) {
    var c = hex(cs.fill);
    if (!c) return null;
    var a = (+cs.fillOpacity) * (+cs.opacity);
    return a < 1 ? { color: c, transparency: Math.round((1 - a) * 100) } : { color: c };
  }

  function lineOf(cs, el) {
    var c = hex(cs.stroke), w = parseFloat(cs.strokeWidth);
    if (!c || !w) return null;
    var ln = { color: c, width: w * PT };
    var a = (+cs.strokeOpacity) * (+cs.opacity);
    if (a < 1) ln.transparency = Math.round((1 - a) * 100);
    if (cs.strokeDasharray && cs.strokeDasharray !== 'none') {
      // Short dashes become PowerPoint's dot preset, longer ones its dash
      ln.dashType = parseFloat(cs.strokeDasharray) <= 4 ? 'dot' : 'dash';
    }
    if (el && cs.markerStart && cs.markerStart !== 'none') ln.beginArrowType = arrow(cs.markerStart);
    if (el && cs.markerEnd && cs.markerEnd !== 'none') ln.endArrowType = arrow(cs.markerEnd);
    return ln;
  }

  // A filled triangle, sized like the SVG marker relative to the line. pptxgenjs has no arrow-size option and
  // writes this value straight into type="…", so the size attributes ride along inside it.
  function arrow(ref) {
    var m = document.getElementById((ref.match(/#([^"')]+)/) || [])[1]);
    var k = m ? m.markerWidth.baseVal.value : 3;
    var size = k <= 2.6 ? 'sm' : k <= 4 ? 'med' : 'lg';
    return 'triangle" w="' + size + '" len="' + size;
  }

  // Element space to the slide's own units, through any <g transform>
  function mapper(svg, el) {
    var m = svg.getScreenCTM().inverse().multiply(el.getScreenCTM());
    return function (x, y) { return [m.a * x + m.c * y + m.e, m.b * x + m.d * y + m.f]; };
  }

  // M/L/H/V/Z and their relative forms, the slide draws straight segments only. One entry per subpath.
  function subpaths(d) {
    var tok = d.match(/[MLHVZmlhvz]|-?[\d.]+(?:e-?\d+)?/g) || [], out = [], cur = null, x = 0, y = 0, cmd = 'M', i = 0;
    while (i < tok.length) {
      if (/[A-Za-z]/.test(tok[i])) {
        cmd = tok[i++];
        if (/z/i.test(cmd)) { if (cur) cur.closed = true; cur = null; continue; }
      }
      var rel = cmd === cmd.toLowerCase(), C = cmd.toUpperCase();
      if (C === 'H') x = (rel ? x : 0) + +tok[i++];
      else if (C === 'V') y = (rel ? y : 0) + +tok[i++];
      else { x = (rel ? x : 0) + +tok[i++]; y = (rel ? y : 0) + +tok[i++]; }
      if (C === 'M' || !cur) { cur = { pts: [], closed: false }; out.push(cur); }
      cur.pts.push([x, y]);
      if (C === 'M') cmd = rel ? 'l' : 'L';
    }
    return out;
  }

  // One shape from a list of slide points, as a line when it is a single segment
  function addPoly(pres, slide, pts, closed, fill, line, name) {
    var xs = pts.map(function (p) { return p[0]; }), ys = pts.map(function (p) { return p[1]; });
    var x0 = Math.min.apply(null, xs), y0 = Math.min.apply(null, ys);
    var w = Math.max.apply(null, xs) - x0, h = Math.max.apply(null, ys) - y0;
    if (pts.length === 2 && !closed) {
      slide.addShape(pres.ShapeType.line, { x: x0 * IN, y: y0 * IN, w: w * IN, h: h * IN, line: line,
        flipH: pts[1][0] < pts[0][0], flipV: pts[1][1] < pts[0][1], objectName: name });
      return;
    }
    var points = pts.map(function (p) { return { x: (p[0] - x0) * IN, y: (p[1] - y0) * IN }; });
    if (closed) points.push({ close: true });
    var o = { x: x0 * IN, y: y0 * IN, w: Math.max(w, 1) * IN, h: Math.max(h, 1) * IN, points: points, objectName: name };
    if (fill) o.fill = fill;
    if (line) o.line = line;
    slide.addShape(pres.ShapeType.custGeom, o);
  }

  function runOpts(cs, base) {
    var mono = /mono|consolas|courier/i.test(cs.fontFamily);
    var o = {
      fontFace: mono ? MONO : SANS,
      fontSize: Math.round(parseFloat(cs.fontSize) * PT * 2) / 2,
      color: hex(cs.fill) || '000000',
      bold: +cs.fontWeight >= 600,
      italic: cs.fontStyle === 'italic',
    };
    var ls = parseFloat(cs.letterSpacing);
    if (ls) o.charSpacing = ls * PT;
    return o;
  }

  function addText(pres, slide, svg, el) {
    var cs = getComputedStyle(el), runs = [], first = true, lineGap = 0;
    // Spacing typed into the text (white-space: pre) is part of the design, so only collapse it otherwise
    var pre = /^pre/.test(cs.whiteSpace);
    el.childNodes.forEach(function (n) {
      var t = n.textContent || '', o;
      if (!pre) t = t.replace(/\s+/g, ' ');
      if (n.nodeType === 3) o = runOpts(cs);
      else if (n.nodeName === 'tspan') {
        var ts = getComputedStyle(n);
        o = runOpts(ts);
        // A lowered tspan is a subscript. Office shrinks subscript text to roughly 60-67 %, so the run is
        // written larger by that much and lands near the size the page draws it at.
        if (+n.getAttribute('dy') > 0 || n.getAttribute('baseline-shift') === 'sub') {
          // pptxgenjs writes baseline × 50: -500 is Office's own -25 % subscript offset; its `subscript` is -40 %
          o.baseline = -500; o.fontSize = Math.round(o.fontSize / 0.62);
        }
        if (+n.getAttribute('dx') > 0 && runs.length && !/ $/.test(runs[runs.length - 1].text)) t = ' ' + t;
        // A tspan placed at its own x starts a new line, the next paragraph in the text box
        if (n.hasAttribute('x') && runs.length) {
          runs[runs.length - 1].options.breakLine = true;
          lineGap = lineGap || +n.getAttribute('dy');
        }
      } else return;
      if (first && !pre) t = t.replace(/^ /, '');
      if (!t) return;
      first = false;
      runs.push({ text: t, options: o });
    });
    if (!runs.length) return;
    if (!pre) runs[runs.length - 1].text = runs[runs.length - 1].text.replace(/ $/, '');

    var map = mapper(svg, el), b = el.getBBox();
    var ax = map(el.x.baseVal.numberOfItems ? el.x.baseVal.getItem(0).value : b.x, 0)[0];
    var top = map(b.x, b.y)[1], h = b.height;
    // Text never wraps in the deck, so the box width only has to place the anchor; the slack absorbs a
    // wider font on the machine that opens it.
    var w = b.width * 1.3 + 24, anchor = cs.textAnchor;
    var x0 = anchor === 'middle' ? ax - w / 2 : anchor === 'end' ? ax - w : ax;
    var opts = {
      x: x0 * IN, y: top * IN, w: w * IN, h: h * IN, margin: 0, wrap: false, valign: 'middle',
      align: anchor === 'middle' ? 'center' : anchor === 'end' ? 'right' : 'left',
      isTextBox: true, objectName: runs.map(function (r) { return r.text; }).join(' ').trim().slice(0, 60),
    };
    if (lineGap) opts.lineSpacing = lineGap * PT;
    slide.addText(runs, opts);
  }

  var images = new Map();

  // Pictures go into the deck as their own bytes, read before the shapes are laid out
  function readImages(svg) {
    images = new Map();
    return Promise.all(Array.prototype.map.call(svg.querySelectorAll('image'), function (el) {
      return fetch(el.href.baseVal).then(function (r) { return r.blob(); }).then(function (b) {
        return new Promise(function (ok) {
          var fr = new FileReader();
          fr.onload = function () { images.set(el, fr.result.replace(/^data:/, '')); ok(); };
          fr.readAsDataURL(b);
        });
      });
    }));
  }

  function walk(pres, slide, svg, node, vb) {
    Array.prototype.forEach.call(node.children, function (el) {
      var tag = el.nodeName, cs = getComputedStyle(el), map, name = el.getAttribute('class') || tag;
      if (tag === 'defs' || tag === 'title' || cs.display === 'none') return;
      if (tag === 'g') return walk(pres, slide, svg, el, vb);
      if (tag === 'text') return addText(pres, slide, svg, el);
      map = mapper(svg, el);
      if (tag === 'image') {
        var ip = map(el.x.baseVal.value, el.y.baseVal.value);
        slide.addImage({ data: images.get(el), x: ip[0] * IN, y: ip[1] * IN, w: el.width.baseVal.value * IN,
          h: el.height.baseVal.value * IN, altText: el.textContent.trim(), objectName: 'picture' });
        return;
      }
      if (tag === 'rect') {
        var x = el.x.baseVal.value, y = el.y.baseVal.value, w = el.width.baseVal.value, h = el.height.baseVal.value;
        // The white sheet behind everything is the slide's own background, not a shape to click past
        if (!x && !y && w >= vb.width && h >= vb.height) return;
        var p = map(x, y), rx = el.rx.baseVal.value, o = { x: p[0] * IN, y: p[1] * IN, w: w * IN, h: h * IN, objectName: name };
        var f = fillOf(cs), l = lineOf(cs);
        if (f) o.fill = f;
        if (l) o.line = l;
        if (rx) o.rectRadius = rx * IN;
        slide.addShape(rx ? pres.ShapeType.roundRect : pres.ShapeType.rect, o);
      } else if (tag === 'circle' || tag === 'ellipse') {
        var rx2 = tag === 'circle' ? el.r.baseVal.value : el.rx.baseVal.value;
        var ry2 = tag === 'circle' ? rx2 : el.ry.baseVal.value, c = map(el.cx.baseVal.value, el.cy.baseVal.value);
        var oc = { x: (c[0] - rx2) * IN, y: (c[1] - ry2) * IN, w: 2 * rx2 * IN, h: 2 * ry2 * IN, objectName: name };
        var fc = fillOf(cs), lc = lineOf(cs);
        if (fc) oc.fill = fc;
        if (lc) oc.line = lc;
        slide.addShape(pres.ShapeType.ellipse, oc);
      } else if (tag === 'line') {
        addPoly(pres, slide, [map(el.x1.baseVal.value, el.y1.baseVal.value), map(el.x2.baseVal.value, el.y2.baseVal.value)],
          false, null, lineOf(cs, el), name);
      } else if (tag === 'polygon' || tag === 'polyline') {
        var pp = Array.prototype.map.call(el.points, function (q) { return map(q.x, q.y); });
        addPoly(pres, slide, pp, tag === 'polygon', fillOf(cs), lineOf(cs), name);
      } else if (tag === 'path') {
        subpaths(el.getAttribute('d')).forEach(function (sp) {
          if (sp.pts.length < 2) return;
          addPoly(pres, slide, sp.pts.map(function (q) { return map(q[0], q[1]); }), sp.closed,
            fillOf(cs), lineOf(cs, el), name);
        });
      }
    });
  }

  function build(svg) {
    return readImages(svg).then(function () { return layout(svg); });
  }

  function layout(svg) {
    var vb = svg.viewBox.baseVal, pres = new window.PptxGenJS();
    pres.defineLayout({ name: 'SLIDE', width: vb.width * IN, height: vb.height * IN });
    pres.layout = 'SLIDE';
    pres.title = document.title;
    var slide = pres.addSlide();
    slide.background = { color: 'FFFFFF' };
    walk(pres, slide, svg, svg, vb);
    // The diagram's spoken description doubles as speaker notes
    if (svg.getAttribute('aria-label')) slide.addNotes(svg.getAttribute('aria-label'));
    return pres;
  }

  window.slidePptx = {
    load: load,
    build: build,
    download: function (svg, fileName) {
      return load().then(function () { return build(svg); })
        .then(function (pres) { return pres.writeFile({ fileName: fileName }); });
    },
  };
})();
