# DESIGN.md — Archaeology Hub

version: 0.1
status: working-design-source-of-truth
product: Archaeology Hub
audience: archaeology field/lab professionals
targets: Windows, macOS, Linux
implementation_target: C++ / Qt 6 / QML
design_basis: Apple Liquid Glass principles + Linear information hierarchy + Raycast utility patterns

---

## 1. Product identity

Archaeology Hub is a professional desktop workspace that groups independent archaeological utilities into one coherent application without loading every tool into a single screen.

The application should feel:
- calm
- precise
- modern
- scientific
- field-ready
- visually lightweight
- information-dense only where the task requires it

The application must NOT feel:
- like a generic SaaS dashboard
- like a gaming launcher
- like an imitation of macOS
- like a website embedded in a desktop shell
- overloaded with glass, gradients, glow, shadows, or decorative cards
- archaeological through clichés such as parchment textures, ruins, dirt, brushes, or faux-aged typography

Core visual idea:

> Content is solid and legible. Navigation and transient controls float above it.

Liquid/glass treatment belongs mainly to navigation, toolbars, command surfaces, inspectors, popovers, and transient controls. The working content itself should normally use solid or standard translucent surfaces.

---

## 2. Design inheritance

This system intentionally combines three design dialects.

### Apple / Liquid Glass
Use for:
- hierarchy between content and navigation
- floating sidebars and toolbars
- adaptive translucency
- rounded control groups
- spacious typography
- responsive window behavior
- accessibility fallbacks

Do NOT use Liquid Glass as a decorative material across every card.

### Linear
Use for:
- information architecture
- restrained dark mode
- surface hierarchy
- data-dense panels
- calm spacing
- status chips
- utility cards
- progressive disclosure
- strong visual consistency

### Raycast
Use for:
- command palette
- fast keyboard navigation
- compact utility rows
- action menus
- recent tools
- quick search
- shortcut presentation
- small, precise controls

---

## 3. Application architecture reflected in the UI

The home screen is a launcher and overview, not a container that renders all tools simultaneously.

Primary structure:

1. App shell
2. Sidebar navigation
3. Home / workspace launcher
4. Tool modules
5. Optional inspector
6. Global command palette
7. Global status / task center

Initial modules:

- Media Transcription
  - audio → transcription
  - video → transcription
  - timestamps
  - minutes / meeting notes
  - export

- Archaeological Catalog
  - photographs
  - finds / materials
  - context metadata
  - file naming
  - batch metadata
  - searchable records

- Coordinate Converter
  - import one coordinate
  - paste table
  - CSV / XLSX import
  - select source CRS
  - select destination CRS
  - preview conversion
  - export
  - recent CRS presets

- Future Tools
  - should plug into the same shell without redesigning the navigation

Do not render the internal UI of inactive modules.

---

## 4. Navigation model

### Desktop shell

Use a three-region layout when space permits:

`Sidebar | Main Content | Optional Inspector`

Recommended widths:
- sidebar expanded: 236–264 px
- sidebar compact: 64–72 px
- inspector: 280–360 px
- main content: fluid
- minimum practical window: ~960 × 640 px

### Sidebar

The sidebar is the principal Liquid Glass surface.

It contains:
- product mark/name
- Home
- Tools
  - Transcription
  - Catalog
  - Coordinates
- Projects / Recent
- bottom utility cluster
  - Settings
  - Help / About

The Tools section may expand/collapse.

Avoid a second permanent navigation bar if the sidebar already expresses location.

### Top toolbar

Use only when the current tool requires contextual actions.

Examples:
- Import
- Export
- New batch
- Undo / redo
- Run conversion
- Search
- Inspector toggle

The top toolbar may float as a glass group over content.

### Breadcrumbs

Use only inside workflows with meaningful hierarchy.

Example:
`Catalog / QA-SYNTHETIC / Pozo 7 / Nivel 3`

Do not use breadcrumbs on the Home screen.

---

## 5. Liquid Glass rules

Glass is a functional layer, never the default content surface.

### Glass is appropriate for

- sidebar
- toolbar
- floating search
- command palette
- popover
- context menu
- transient action tray
- inspector header
- compact status/task overlay

### Glass is NOT appropriate for

- large data tables
- catalog records
- long forms
- report text
- coordinate tables
- metadata editors
- main work canvas
- persistent content cards

### Glass Regular

Use for:
- sidebar
- command palette
- popovers containing text
- toolbars
- inspector navigation

Visual target:
- adaptive translucent fill
- strong enough background diffusion to preserve text contrast
- subtle 1 px edge highlight
- soft separation from content
- no exaggerated glow

Approximate fallback tokens:

Light:
- glass-fill: rgba(248, 250, 252, 0.72)
- glass-fill-strong: rgba(248, 250, 252, 0.84)
- glass-border: rgba(255, 255, 255, 0.52)
- glass-separator: rgba(20, 24, 28, 0.10)

Dark:
- glass-fill: rgba(26, 29, 33, 0.70)
- glass-fill-strong: rgba(29, 32, 36, 0.84)
- glass-border: rgba(255, 255, 255, 0.12)
- glass-separator: rgba(255, 255, 255, 0.10)

Suggested blur fallback:
- 18–24 px visual blur equivalent
- saturation boost only if subtle
- avoid blur when performance or platform support is poor

### Glass Clear

Use only over visually rich imagery or a map/canvas where preserving background visibility is useful.

Never use clear glass for dense text.

### Reduced transparency fallback

When transparency or blur is disabled:
- replace glass with an opaque elevated surface
- preserve radius, spacing, border hierarchy, and text contrast
- do not rely on transparency to communicate selection

---

## 6. Color system

The product uses neutral surfaces plus one restrained archaeological/scientific accent.

### Primary accent — Mineral Teal

Light mode:
- accent-primary: #0B7F6F
- accent-hover: #096E61
- accent-soft: rgba(11, 127, 111, 0.12)

Dark mode:
- accent-primary: #4ED8C1
- accent-hover: #69E2CE
- accent-soft: rgba(78, 216, 193, 0.14)

Use accent for:
- primary action
- active navigation marker
- focus ring
- selected toggle
- progress completion
- links where a link color is needed

Do not use the accent as large section backgrounds.

### Light surfaces

- canvas: #F5F6F7
- content: #FFFFFF
- surface-1: #FAFAFB
- surface-2: #F0F2F4
- surface-3: #E8EBEE
- ink: #15171A
- body: #3F444B
- muted: #747B84
- disabled: #A3A8AE
- hairline: rgba(20, 24, 28, 0.10)
- hairline-strong: rgba(20, 24, 28, 0.18)

### Dark surfaces

Avoid absolute black.

- canvas: #0B0C0E
- content: #111317
- surface-1: #15181C
- surface-2: #1B1F24
- surface-3: #23282E
- ink: #F4F6F8
- body: #CDD2D8
- muted: #949BA4
- disabled: #626A73
- hairline: rgba(255, 255, 255, 0.09)
- hairline-strong: rgba(255, 255, 255, 0.16)

### Semantic colors

Use semantic colors only for meaning.

- success: #2E9B66
- warning: #B97812
- error: #C94A4A
- info: #3D7FC4

Each semantic color should have a low-opacity soft background variant for badges and inline notices.

---

## 7. Typography

Prefer system fonts to preserve native rendering.

Font stack concept:
- macOS: SF Pro / system UI
- Windows: Segoe UI Variable / system UI
- Linux: Inter or platform system UI
- monospace: SF Mono / Cascadia Mono / JetBrains Mono / system monospace

Do not bundle proprietary fonts.

### Type scale

- display: 32 px / 600 / 1.15
- heading-1: 26 px / 600 / 1.20
- heading-2: 20 px / 600 / 1.25
- heading-3: 17 px / 600 / 1.30
- body-lg: 16 px / 400 / 1.50
- body: 14 px / 400 / 1.50
- body-strong: 14 px / 600 / 1.45
- caption: 12 px / 400 / 1.40
- label: 12 px / 600 / 1.30
- mono: 13 px / 400 / 1.45

Guidelines:
- use 600 instead of 700 for normal headings
- reserve monospace for coordinates, IDs, filenames, CRS codes, logs, and timestamps
- use tabular numerals for tables, coordinates, measurements, counts, and timestamps
- avoid uppercase except for tiny taxonomy labels, CRS codes, or conventional abbreviations

---

## 8. Radius system

- xs: 6 px
- sm: 8 px
- md: 12 px
- lg: 16 px
- xl: 22 px
- pill: 999 px

Use:
- 8 px: compact controls
- 12 px: inputs and normal cards
- 16 px: major panels
- 22 px: floating glass groups
- pill: chips, segmented filters, status capsules

Do not make every element pill-shaped.

---

## 9. Spacing system

Base rhythm: 4 px

Tokens:
- 4
- 8
- 12
- 16
- 20
- 24
- 32
- 40
- 48
- 64

Common usage:
- compact row vertical padding: 8 px
- button horizontal padding: 14–18 px
- card padding: 16–24 px
- panel padding: 24 px
- tool page top spacing: 24–32 px
- major section spacing: 40–64 px

Desktop utilities should be compact without becoming cramped.

---

## 10. Depth and elevation

Primary hierarchy should come from:
1. position
2. surface tone
3. blur/translucency
4. border
5. shadow, only when needed

### Elevation 0
Flat content.

### Elevation 1
1 px hairline.

### Elevation 2
Floating glass or popover:
- soft edge shadow
- low opacity
- broad blur
- no dark dramatic shadow

### Elevation 3
Modal / command palette:
- stronger but still soft shadow
- optional subtle backdrop dimming

Avoid shadows on every card.

---

## 11. Home screen

The Home screen is a launcher.

Recommended structure:

Header:
- “Archaeology Hub”
- short contextual subtitle
- global search / command trigger

Primary section:
- Continue working
- 1–3 recent projects/tasks maximum

Tools section:
- clean grid or list of available modules

Each tool entry contains:
- icon
- tool name
- one-line purpose
- optional recent state
- shortcut if assigned

Suggested tool icons:
- Transcription: waveform / captions
- Catalog: grid / archive box / photo stack
- Coordinates: coordinate crosshair / map pin / axes

Do not place technical settings on Home.

---

## 12. Tool page pattern

Every tool should follow the same internal structure:

1. Tool title
2. compact action toolbar
3. primary workspace
4. optional inspector
5. status / progress area

Avoid wrapping every region in a card.

Use containers only where they communicate grouping.

---

## 13. Coordinate Converter UI

This module is data-centric.

### Layout

Top action row:
- Add coordinate
- Paste data
- Import file
- Recent preset

Conversion setup:
- Source CRS
- Destination CRS
- axis/order indicator
- units indicator

Main table:
- input columns
- parsed status
- converted columns
- validation state

Bottom or top-right primary action:
- Convert

Export actions:
- Copy
- CSV
- XLSX
- GeoJSON
- optional GIS formats if implemented

### CRS selector

Use searchable picker.

Each CRS result may show:
- human-readable name
- EPSG code when applicable
- geographic/projected indicator
- unit
- area/use note

Prioritize commonly used Costa Rican archaeological systems as pinned/recent presets, but keep the architecture extensible.

Coordinates and EPSG identifiers use monospace.

Invalid coordinates should be visibly identified by row, not only by a global error message.

---

## 14. Catalog UI

The catalog should support both browsing and batch work.

Preferred desktop structure:
- list/grid browser
- metadata inspector
- image preview

View modes:
- compact list
- thumbnail grid
- detail

High-value metadata should remain scannable:
- project/site
- context
- unit/pozo
- level
- artifact/material category
- filename
- date
- notes

Batch editing should use a dedicated action bar that appears only when multiple records are selected.

Images remain content; do not place heavy glass over them except for transient controls.

---

## 15. Transcription UI

Use a document/editor model.

Structure:
- source media region
- transcript editor
- timeline/timestamps
- export actions

Progress states:
- queued
- processing
- transcribing
- generating summary/minutes
- complete
- error

Keep progress states semantic and compact.

Do not animate large decorative loaders.

---

## 16. Tables

Tables are a first-class component.

Requirements:
- row height: 36–44 px depending on density
- sticky header where useful
- subtle separators
- selected row uses accent-soft, not a saturated fill
- hover state should remain subtle
- numeric columns right-aligned where appropriate
- coordinates/IDs use monospace
- preserve keyboard navigation
- provide column resize when useful
- allow horizontal scrolling rather than crushing data

Do not use rounded cards for every table row.

---

## 17. Inputs

Default height:
- compact desktop: 36 px
- touch-oriented / large: 44 px

Styles:
- solid or standard translucent content surface
- 1 px hairline
- 8–12 px radius
- visible focus ring using accent
- inline validation message when necessary

Labels belong above or beside fields according to available width.

Never rely on placeholder text as the only label.

---

## 18. Buttons

### Primary
Use accent fill.

Use only for the dominant action in the current context.

Examples:
- Convert
- Start transcription
- Save
- Export final

### Secondary
Neutral elevated surface with hairline.

### Tertiary
Text/icon action.

### Destructive
Semantic error color, only for destructive actions.

### Icon buttons
- compact: 32–36 px
- touch/important: 44 px
- tooltip required if meaning is not obvious

Avoid multiple primary buttons in the same action cluster.

---

## 19. Segmented controls, chips, and badges

Segmented control:
- use for 2–4 mutually exclusive views
- selected state uses surface lift or accent-soft
- avoid a strong filled accent unless action significance requires it

Chips:
- filters
- CRS presets
- metadata taxonomy

Badges:
- status only
- compact
- never decorative

---

## 20. Command palette

Keyboard shortcut:
- Ctrl/Cmd + K

Appearance:
- centered floating Glass Regular panel
- width ~560–680 px
- maximum height ~65% of window
- 16–22 px outer radius
- search field integrated at top
- grouped action results
- compact rows
- shortcut hints aligned right

Capabilities:
- switch tool
- open recent project
- run frequent action
- search commands
- open settings
- jump to coordinate converter
- create catalog batch
- import media

Raycast-like behavior is useful; visual cloning is not.

---

## 21. Keyboard-first behavior

Desktop workflows must remain efficient without mouse-only interaction.

Recommended defaults:
- Ctrl/Cmd + K: command palette
- Ctrl/Cmd + O: open/import where contextually safe
- Ctrl/Cmd + S: save
- Ctrl/Cmd + E: export when appropriate
- Ctrl/Cmd + F: search within current tool
- Ctrl/Cmd + ,: settings
- Escape: close transient surface
- Enter: activate primary row/action
- Space: preview when relevant

Shortcuts must be discoverable through menus/tooltips.

---

## 22. Motion

Motion is functional.

Durations:
- micro state: 100–140 ms
- panel transition: 160–220 ms
- modal/popover: 180–240 ms

Use:
- opacity
- small translation
- subtle scale for transient floating surfaces

Avoid:
- bouncing
- elastic overshoot
- constant animated backgrounds
- glass shimmer
- parallax in productivity views

Respect reduced-motion settings.

---

## 23. Responsive desktop behavior

This is a desktop-first cross-platform application, but windows can become narrow.

### ≥ 1280 px
- expanded sidebar
- main content
- inspector if requested

### 960–1279 px
- expanded or compact sidebar
- inspector may overlay or collapse

### 720–959 px
- compact sidebar
- single primary content region
- inspector becomes temporary drawer

### < 720 px
- navigation becomes temporary overlay/drawer
- toolbars condense to essential actions
- secondary actions move to overflow
- tables preserve horizontal scrolling

Do not simply scale everything down.

---

## 24. Accessibility

Must support:
- keyboard navigation
- visible focus indication
- reduced motion
- reduced transparency / blur fallback
- high contrast modes where available
- scalable text
- non-color status cues
- tooltips for ambiguous icons
- sufficient text/background contrast
- error text paired with icon/state, not color alone

Glass effects may never be required to understand hierarchy or status.

---

## 25. Empty, loading, error, and offline states

### Empty
Explain what the tool expects and give one clear next action.

### Loading
Prefer inline progress to blocking modal spinners.

### Long-running jobs
Use task center / progress panel.
Allow the user to continue working where safe.

### Error
State:
- what failed
- affected item
- whether data is preserved
- retry or recovery action

Avoid generic “Something went wrong” when a useful error can be shown.

---

## 26. Status/task center

Long-running operations should appear in a compact global task center.

Examples:
- transcription
- image/catalog import
- coordinate batch conversion
- export

Each task:
- icon
- name
- progress
- state
- optional cancel/retry
- completion result

The task center may be a floating Glass Regular popover opened from the toolbar/status area.

---

## 27. Icons

Use a coherent outline icon set.

Characteristics:
- simple geometry
- 1.5–2 px optical stroke
- rounded joins where suitable
- filled variants only for selected states when the icon set supports it

Do not mix unrelated icon families.

Platform-native symbols can be used when available, but maintain semantic parity across platforms.

---

## 28. Archaeological visual language

Archaeology should appear through:
- terminology
- domain workflows
- coordinate/data presentation
- context hierarchy
- photography
- maps
- scientific metadata
- restrained mineral/earth semantic accents where meaningful

Avoid:
- parchment
- sepia UI
- excavation-tool clip art
- faux ancient fonts
- distressed textures
- ruins as generic background decoration

The application is a modern archaeological instrument, not a museum-themed website.

---

## 29. Platform adaptation

The app should share one design system while respecting each OS.

### macOS
- use native-feeling window spacing
- traffic-light/titlebar region must not be visually crowded
- system font
- platform-appropriate shortcuts

### Windows
- use system font
- support native window behavior and scaling
- do not imitate macOS titlebar controls
- maintain glass-like hierarchy through Qt effects/fallback surfaces

### Linux
- use available system font or Inter fallback
- do not assume blur/compositor support
- fall back cleanly to opaque elevated surfaces
- preserve identical information architecture

The goal is “same product, native-respectful implementation,” not pixel-identical OS imitation.

---

## 30. Qt / QML implementation guidance

This section is implementation guidance, not a requirement to hard-code visual behavior.

Prefer:
- reusable design tokens exposed centrally
- semantic component properties
- Qt Quick Controls with custom styling
- shared QML components
- platform capability detection
- GPU-friendly effects
- minimal nested blur layers

Avoid:
- hard-coded hex values inside individual screens
- duplicate button/input implementations
- blur on large scrolling content regions
- stacking multiple transparent surfaces
- loading every module at startup

Suggested component vocabulary:

- AppShell
- Sidebar
- SidebarItem
- ToolPage
- ToolHeader
- GlassToolbar
- GlassPopover
- InspectorPanel
- PrimaryButton
- SecondaryButton
- IconButton
- SearchField
- DataTable
- StatusBadge
- SegmentedControl
- CommandPalette
- TaskCenter
- EmptyState
- InlineNotice
- CoordinateField
- CrsPicker

---

## 31. Performance rules

Visual effects must not compromise tool responsiveness.

Prefer:
- one major blur region rather than many nested blur regions
- lazy module loading
- virtualization for long lists/tables
- thumbnail caching
- asynchronous file parsing
- background processing for transcription/import/conversion
- progressive rendering

If blur causes visible frame drops, use the opaque fallback surface.

---

## 32. Do

- Keep main work surfaces solid and clear.
- Use glass mostly for functional/navigation layers.
- Keep one primary accent.
- Use surface hierarchy before adding shadows.
- Make tables and metadata first-class.
- Keep navigation consistent across tools.
- Use progressive disclosure.
- Support keyboard-driven workflows.
- Preserve recent projects and recent tool context.
- Make batch operations explicit.
- Use responsive layout changes, not simple shrinking.
- Prefer system fonts.
- Keep status and progress visible but quiet.
- Use one shared token system across all modules.

---

## 33. Do not

- Do not put every module on the Home screen at once.
- Do not apply Liquid Glass to every card.
- Do not use strong gradients as general decoration.
- Do not use neon glows.
- Do not use absolute black for the main dark canvas.
- Do not make every container rounded.
- Do not hide important data behind hover-only interactions.
- Do not depend on color alone for status.
- Do not imitate macOS window controls on Windows/Linux.
- Do not use archaeological clichés as UI decoration.
- Do not let each module invent its own layout system.
- Do not load inactive heavy modules just because they are visible in navigation.

---

## 34. Visual acceptance checklist

Before merging a UI change, verify:

- Does the screen have one obvious primary task?
- Is navigation visually distinct from content?
- Is glass limited to functional layers?
- Can the screen still work with transparency disabled?
- Is the primary accent scarce?
- Are tables readable without card-per-row styling?
- Are typography and spacing using tokens?
- Are controls keyboard accessible?
- Are long-running tasks represented non-blockingly?
- Does the layout work at narrow desktop widths?
- Does dark mode use surface steps instead of pure black?
- Does the interface look professional without relying on decorative imagery?
- Does the tool look like part of Archaeology Hub rather than a separate app?

---

## 35. Agent instructions

When modifying or creating UI:

1. Read this entire DESIGN.md first.
2. Treat these rules as the visual source of truth.
3. Reuse existing shared components before adding a new one.
4. Prefer semantic tokens to literal colors/sizes.
5. Do not redesign unrelated screens.
6. Do not add decorative glass simply to make a view look “premium.”
7. Keep domain data legible and operationally useful.
8. Preserve cross-platform behavior.
9. When a design choice conflicts with data readability, readability wins.
10. When a design choice conflicts with platform accessibility, accessibility wins.

For new modules:
- inherit AppShell
- register one navigation item
- lazy-load the module
- use ToolPage structure
- use shared actions/components
- document any genuinely new reusable component

---

## 36. Codex design task prompt

Use this DESIGN.md as the visual source of truth for Archaeology Hub.

Do not attempt a full redesign in one pass.

First:
1. audit the existing UI and identify reusable screens/components,
2. create the central token/theme layer,
3. create the AppShell + Sidebar + Home launcher,
4. integrate existing tools one at a time behind lazy-loaded routes,
5. implement the command palette,
6. apply glass only to navigation/transient functional surfaces,
7. preserve existing tool behavior while migrating visual structure.

The application is desktop-first, C++/Qt 6/QML, cross-platform for Windows, macOS, and Linux.

Prioritize maintainability, native-respectful behavior, performance, data readability, and accessibility over visual effects.
