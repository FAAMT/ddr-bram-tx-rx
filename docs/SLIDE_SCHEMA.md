# Slide schema

Every slide made from this repo follows this page. It is drawn from the lab's own
slide template: the palette, the zone-and-card layout, the connector language and
the legend row are taken from it. The institution's logo and banner are not
reproduced. The space they need is kept empty instead, so the banner can be added
afterwards without anything moving.

`web/brief.html` is the reference implementation. Its `:root` block holds the
tokens below verbatim.

## Page

| | |
|---|---|
| format | 16:9 landscape, the Google Slides default: 10 × 5.625 in, 960 × 540 pt |
| authoring size | 1280 × 720 px; 1 px = 0.75 pt |
| print | `@page { size: 1280px 720px; margin: 0 }`, one slide per page, the PDF from headless chromium |
| background | white, always |
| side margins | 56 px |
| **banner reserve** | **the bottom 72 px (10 %) stays empty.** Nothing is drawn there: no footer, no page number, no legend. The template's banner (a thin stripe over a solid band, logo left, page number right) goes there later |

Vertical bands, in px from the top:

| band | y | holds |
|---|---|---|
| title | 32 – 104 | title, left; one status pill, right |
| rule | 112 | 1.5 px line in `--rule`, margin to margin |
| content | 128 – 596 | zones |
| legend | 604 – 640 | the legend row |
| banner reserve | 648 – 720 | nothing |

## Type

- **Font:** Franklin Gothic, as the template requires. Where it is not installed,
  Libre Franklin from Google Fonts stands in; the stack is
  `'Franklin Gothic Book', 'Franklin Gothic Medium', 'Libre Franklin', Arial, sans-serif`.
- **Floor: 16 pt, which is 22 px.** It applies to every word on the slide, diagram
  labels and legend included. The reference template runs its card subtitles near
  10 pt; that is not copied. If text does not fit at 22 px, cut words. Do not
  shrink the type.

| role | size | weight | colour |
|---|---|---|---|
| title | 40 px (30 pt) | 400 | `--title` |
| zone header | 22 px | 700 | the zone's accent |
| zone descriptor | 22 px | 400 italic | `--sub` |
| card title | 22 px | 700 | `--ink` |
| card subtitle, body | 22 px | 400 | `--sub` |
| status line in a card | 22 px | 700 italic | `--teal` |
| equation | 34 px | 600 italic | `--ink`, variables in the accent |
| legend | 22 px | 400 | `--sub` |

The title is sentence case or title case, never all caps, and never bold.

## Colour

Three accents, with a role each, plus their tints and a neutral ramp. No other
hues are used.

| token | hex | role |
|---|---|---|
| `--purple` | `#4f0f87` | context and the platform: zone headers for existing systems, icons, RF links, open-question outlines |
| `--teal` | `#35746e` | our contribution: the focus zone, the focal card, the status pill, *working* or *in progress* |
| `--rust` | `#b34a22` | risk: interference, a blocker, anything that can stop the work |
| `--purple-tint` | `#f4f0f9` | zone fill for context zones |
| `--teal-tint` | `#e8f2f1` | zone fill for the focus zone |
| `--rust-tint` | `#fbf1eb` | card fill for a risk card |
| `--rust-line` | `#ebcab6` | border of a risk card |
| `--title` | `#3f3f3f` | slide title |
| `--ink` | `#262626` | card titles, table values |
| `--sub` | `#5f5f5f` | subtitles, descriptors, legend |
| `--wire` | `#8c8c8c` | wired and IQ connectors, control connectors, *not started* |
| `--rule` | `#7f7f7f` | the rule under the title |
| `--card-line` | `#d8d2e2` | card borders |
| `--white` | `#ffffff` | page and card fill |

Banner colours `#86308b` (stripe) and `#4f0f87` (band) belong to the banner, not
to content. Magenta never appears above the reserve.

## Layout patterns

**Zones.** The content band is split into two or three zones: rounded panels
(radius 16 px) with a tinted fill and no border. Each has a header line, the
zone name in the accent colour and then an italic descriptor in `--sub`, for
example *Digital channel emulation · Live IQ, repeatable*. At most one zone is
the focus zone (teal tint). The rest are context zones (purple tint).

**Cards.** White fill, a 1.5 px `--card-line` border, radius 12 px. A card holds
a title and at most one subtitle line. It can have a line icon, either above the
title (centred card) or to its left (wide card).

| card | how it differs |
|---|---|
| focal | 2 px `--teal` border, a filled teal circle icon with a white glyph, an optional italic teal status line. One per slide |
| risk | `--rust-tint` fill, `--rust-line` border, a rust icon |
| open question | 2 px dashed `--purple` border. The legend says "Dashed box: open question" |
| status | a 5 px left edge in the status colour, a glyph, a title, then one line of detail |

**Icons.** Line icons on a 24-unit grid with a 2-unit stroke, round caps and
joins, in `--purple`, or white on the teal circle. The style is Tabler Icons.
Draw them inline. One icon per card, and no decorative icons.

**Status pill.** Optional, at the right of the title: a teal-filled rounded pill
with white bold text, saying what stage the slide shows (*In progress*, *Then
add*, *Preliminary*).

## Connectors

The meaning comes from the line, not from labels on it.

| line | means |
|---|---|
| purple, 4 px, solid, double-headed | RF, over the air |
| `--wire`, 2.5 px, solid | wired or IQ data |
| `--wire`, 2 px, dashed | control or measurements |
| rust, 2 px, dashed | interference or a fault injected |

Arrowheads are filled triangles in the line's colour. Lines run orthogonally,
with no curves.

## Legend row

Required whenever the slide uses colour or line style to mean something. That is
almost always. It sits in the legend band, one row, from margin to margin, items
spaced evenly. Each item is a short sample of the line, or a 18 px swatch, then
its label. List only what the slide actually uses.

## Rules carried from the briefing guidance

- No animation and no transitions; slides are printed as read-aheads.
- A graphic or table that uses colour coding carries its legend.
- Graphical forms over prose wherever they explain more.
- Nothing that is IP or "secret sauce"; uses are commercial only.
- Preliminary results are welcome and are labelled as preliminary.

## Checklist before sending

1. One PDF page per slide, 960 × 540 pt (`pdfinfo`).
2. The bottom 72 px is empty in the rendered page image.
3. No text under 22 px: check the stylesheet and every SVG `font-size`.
4. Only the tokens above appear in the stylesheet.
5. Every colour and line style on the slide is in the legend row.
6. On screen, the page passes the phone, tablet and desktop checks.
