# TIFF Support in Chromium

**User:** Milan Hauth (milahu@gmail.com)  
**Created:** 2026/4/20 18:11:54  
**Updated:** 2026/4/20 19:42:42  
**Exported:** 2026/4/20 19:52:28  
**Link:** [https://chatgpt.com/c/69e6501d-69e0-83eb-bc60-8a47f4cf08b9](https://chatgpt.com/c/69e6501d-69e0-83eb-bc60-8a47f4cf08b9)  

## Prompt:
lets say i want to add TIFF image format support to the chromium web browser.
how much work would that be?
what parts of the codebase would i have to modify?
can i use a tiff parsing library to save work?
can i use other libraries to support some tiff extensions (CCITT T.4 bi-level encoding, CCITT T.6 bi-level encoding, LZW, JPEG)?

## Response:
Short answer: this is a **non-trivial, multi-month project** even for an experienced Chromium contributor. It touches decoding, rendering, security review, and cross-platform plumbing. It’s not “add a parser and you’re done.”

Let’s break it down realistically.

* * *

🧱 What “TIFF support” actually means in a browser
--------------------------------------------------

Chromium already supports formats like PNG/JPEG via its image pipeline (mainly in **Blink + Skia + image decoders**). To add TIFF, you’d need:

1.  **Decode TIFF → raw bitmap**
2.  **Integrate that decoder into Chromium’s image pipeline**
3.  **Handle edge cases** (multi-page TIFFs, color profiles, weird encodings)
4.  **Pass security and performance constraints**

TIFF is _much_ more complex than PNG/JPEG. It’s basically a container format with dozens of variations.

* * *

🔧 Parts of Chromium you’d touch
--------------------------------

### 1\. Image decoding layer (core work)

*   `//third_party/blink/renderer/platform/image-decoders/`
*   `//ui/gfx/codec/`

You’d likely:

*   Add a new decoder class (e.g. `TiffImageDecoder`)
*   Hook it into Blink’s image loading pipeline

This is where most of the complexity lives.

* * *

### 2\. Skia integration

Chromium uses **Skia** for rendering.

*   You need to convert decoded TIFF data into `SkBitmap` / `SkImage`
*   Ensure correct color space handling (ICC profiles, etc.)

* * *

### 3\. MIME type + loading pipeline

*   Register `image/tiff`
*   Update:
    *   `mime_util`
    *   network stack handling
    *   `<img>` tag support in Blink

* * *

### 4\. Multi-frame / animated handling

TIFF can contain:

*   multiple pages
*   pyramidal images (used in medical/imaging)

Chromium would need to decide:

*   show first frame only?
*   expose pages like GIF frames?

* * *

### 5\. Security hardening

This is **huge**:

*   Image decoders are a major attack surface
*   You’ll need:
    *   fuzzing
    *   sandbox compatibility
    *   integer overflow protection
    *   OOM handling

Chromium has strict requirements here.

* * *

📦 Can you use an existing TIFF library?
----------------------------------------

Yes—and you probably should.

The obvious candidate is:

*   **libtiff**

### Pros:

*   Mature
*   Supports many TIFF variants
*   Saves massive development time

### Cons:

*   Large attack surface
*   Historically security-sensitive
*   May not meet Chromium’s sandboxing/perf expectations out of the box

👉 Chromium often prefers:

*   **hardened, minimal decoders**
*   or **custom wrappers around third-party libs**

So you’d likely:

*   sandbox it heavily
*   or strip it down

* * *

🧩 Supporting TIFF compressions
-------------------------------

You asked about specific encodings:

### ✔️ CCITT T.4 / T.6 (fax encodings)

*   Supported by libtiff
*   Used in bilevel (black/white) images
*   Implementation complexity: **medium**

* * *

### ✔️ LZW

*   Also supported by libtiff
*   Patent issues are no longer relevant
*   Complexity: **low (if using a library)**

* * *

### ✔️ JPEG-in-TIFF

*   Trickier
*   Requires integration with JPEG decoder (Chromium already has one)

Potential issue:

*   inconsistent implementations across TIFF files

* * *

### ⚠️ Reality check

TIFF supports far more:

*   tiled images
*   planar vs chunky formats
*   weird bit depths (1, 4, 12, 16, 32)
*   floating point images
*   CMYK, Lab, etc.

Supporting “just a subset” is possible—but:

*   you must **gracefully reject unsupported variants**

* * *

⏱️ Rough effort estimate
------------------------

### Minimal (basic TIFF support)

*   single image
*   common encodings (LZW, maybe JPEG)
*   no advanced features

👉 **2–4 months** for an experienced engineer

* * *

### Production-quality Chromium feature

*   secure
*   performant
*   fuzzed
*   cross-platform
*   supports many variants

👉 **6–12+ months**, possibly more

* * *

🧠 Biggest hidden challenges
----------------------------

*   Security review (Chromium is extremely strict)
*   Memory usage (TIFFs can be huge)
*   Incremental decoding (for large images)
*   Color management correctness
*   Handling malformed files safely

* * *

🧭 Practical advice
-------------------

If your goal is experimentation:

*   Start by wrapping **libtiff**
*   Implement a minimal decoder
*   Display via Skia

If your goal is _upstream Chromium_:

*   Expect heavy code review
*   Expect to justify:
    *   why TIFF should be supported (it currently isn’t for good reasons)
    *   security posture
    *   maintenance burden

* * *

⚖️ Final reality check
----------------------

This isn’t like “add WebP support” (which Chromium already did years ago). TIFF is:

*   fragmented
*   complex
*   rarely used on the web
*   high-risk from a security perspective

That’s exactly why Chromium doesn’t natively support it today.

* * *

If you want, I can sketch a **minimal prototype architecture** or show how to wire a basic decoder into Blink step-by-step.

## Prompt:
\* or strip it down

yes, i prefer a stripped-down (minimal) version of libtiff for optimal performance. do such projects already exist?

> 6–12+ months, possibly more

what part of this project would take the most time?
is it all the security testing and patching?

## Response:
You’re thinking in exactly the right direction: **“strip libtiff down to the minimum viable subset”** is the only sane way to approach this for Chromium.

Let’s tackle your two questions separately.

* * *

🧩 1. Do minimal / stripped TIFF decoders already exist?
========================================================

Yes—but with an important caveat: **they’re all _very limited_**. That’s actually a feature, not a bug.

### Notable examples

### 1\. TinyTIFF (very minimal, high-performance)

*   **TinyTIFF**
*   Designed explicitly as a **replacement for libtiff in constrained use cases**

**What it supports:**

*   Basic TIFF structure
*   Fast I/O
*   Multi-frame (with constraints)

**What it _does NOT_ support:**

*   Compression (big one!)
*   Full TIFF feature set

👉 It intentionally drops complexity to gain speed and simplicity [jkriege2.github.io](https://jkriege2.github.io/TinyTIFF/?utm_source=chatgpt.com)

* * *

### 2\. TinyDNG (header-only, limited TIFF subset)

*   **TinyDNG**

**What it supports:**

*   Basic TIFF reading
*   Some compression (lossless JPEG, ZIP)
*   8/16/32-bit images

**What it _does NOT_ support:**

*   Full TIFF spec
*   Many compression formats

👉 It’s explicitly “simple & limited” and even fuzz-tested for safety [GitHub](https://github.com/syoyo/tinydng?utm_source=chatgpt.com)

* * *

### 3\. “Roll your own minimal decoder” (very common in practice)

This is actually what many projects do:

*   Parse:
    *   header
    *   IFD (Image File Directory)
*   Support only:
    *   baseline tags
    *   1–2 compression formats
*   Reject everything else

👉 This is how formats like PNG/WebP decoders in browsers evolved: **tight, purpose-built implementations**

* * *

⚠️ Key takeaway
---------------

There is **no widely-used “secure minimal TIFF decoder” equivalent to libpng/libjpeg**.

You basically have three choices:

| Approach | Reality |
| --- | --- |
| Use **libtiff** | Feature-complete but heavy + risky |
| Use TinyTIFF / TinyDNG | Lightweight but incomplete |
| Write your own subset | Most control, most work |

* * *

⏱️ 2. What actually takes the most time?
========================================

You guessed it—and yes, but not _only_ that.

🥇 The biggest time sink is:
----------------------------

### **“Defining and enforcing a safe subset of TIFF”**

Not just testing.

* * *

Breakdown of where time really goes
-----------------------------------

### 1\. 🔥 Format complexity (BIGGEST hidden cost)

TIFF is not one format—it’s a **family of formats**:

*   Dozens of tags
*   Multiple storage layouts (strip vs tile)
*   Many compressions
*   Arbitrary metadata placement

Even **libtiff itself exposes multiple abstraction layers** because of this complexity [GitLab](https://libtiff.gitlab.io/libtiff/libtiff?utm_source=chatgpt.com)

👉 The hardest part is deciding:

> “What do we support, and what do we reject?”

That design alone can take weeks.

* * *

### 2\. 🔐 Security hardening (yes, this is huge)

You’re right to call this out.

This includes:

*   fuzzing (months, ongoing)
*   fixing:
    *   integer overflows
    *   OOB reads
    *   decompression bombs
*   memory limits (TIFF can request huge allocations)

Even **libtiff had to remove features due to security issues and lack of maintenance** [GitLab](https://libtiff.gitlab.io/libtiff/releases/v4.6.0.html?utm_source=chatgpt.com)

👉 In Chromium, this is **non-negotiable** and often the longest phase.

* * *

### 3\. 🧠 Decompression support (surprisingly painful)

Each compression adds a mini-project:

*   CCITT T.4 / T.6 → bit-level decoding
*   LZW → dictionary + edge cases
*   JPEG-in-TIFF → integration mismatch issues (and real bugs exist) [about.gitlab.com](https://gitlab.com/libtiff/libtiff/-/issues/635?utm_source=chatgpt.com)

👉 Supporting multiple compressions multiplies complexity.

* * *

### 4\. 🧪 Real-world compatibility

You’ll hit files that:

*   violate the spec
*   rely on quirks
*   mix features in weird ways

👉 This is where most “simple decoders” break.

* * *

### 5\. 🧵 Chromium integration (non-trivial but not dominant)

*   Blink decoder wiring
*   Skia conversion
*   threading / sandboxing

👉 Important, but not the main time sink.

* * *

🎯 So what actually dominates the timeline?
===========================================

If I rank it realistically:

1.  **Choosing + implementing a safe subset**
2.  **Security hardening & fuzzing**
3.  **Compression support**
4.  Real-world compatibility fixes
5.  Chromium integration

* * *

🧠 Strategic advice (based on what you said)
============================================

If your goal is **“minimal + performant”**, the winning strategy is:

### ✅ Start with a _very narrow scope_

Support only:

*   Baseline TIFF
*   8-bit RGB or grayscale
*   Strip-based images
*   ONE compression (e.g. LZW or none)

Reject everything else.

* * *

### ✅ Either:

*   Fork **libtiff** and compile out features via config flags (it’s designed for this [GitLab](https://libtiff.gitlab.io/libtiff/internals.html?utm_source=chatgpt.com))

**OR**

*   Start from something like **TinyDNG** and extend

* * *

### ❌ Avoid:

*   “Full TIFF support but optimized”  
    → that’s basically reimplementing libtiff (multi-year effort)

* * *

💬 Bottom line
==============

*   Yes, minimal TIFF decoders exist—but they’re intentionally incomplete
*   The hardest part is **not coding**, it’s **defining a safe, limited subset and making it robust**
*   Security work is a _huge_ chunk—but it’s tightly coupled with format complexity, not separate

* * *

If you want, I can outline a **concrete “Chromium-friendly minimal TIFF spec”** (like: exact tags, compressions, limits) that would keep this project manageable.

## Prompt:
> Start with a very narrow scope

yes. i need the TIFF format as a container for CCITT-Group4 compressed images with a single strip and only one image, so i would focus on that, and exclude all other features.
do you know a minimal TIFF library i can use for this purpose?
or do i have to use the main libtiff and disable unused features?

## Response:
What you’re describing—**“TIFF as a thin wrapper around a single-strip CCITT Group 4 image”**—is about the _simplest useful subset_ of TIFF you can define. That’s good news: it dramatically reduces the problem.

But here’s the blunt reality:

> There is no widely-used, production-ready **minimal TIFF library specifically for CCITT Group 4 only**.

You’re in a niche corner.

* * *

🧩 Your exact target subset (very constrained)
==============================================

You’ve effectively defined:

*   Compression: **CCITT T.6 (Group 4)**
*   Layout: **single strip**
*   Images: **exactly one**
*   Pixel format: **1-bit (bi-level)**
*   Likely tags:
    *   `ImageWidth`
    *   `ImageLength`
    *   `Compression = 4`
    *   `StripOffsets`
    *   `StripByteCounts`
    *   `RowsPerStrip`
    *   maybe `PhotometricInterpretation`

👉 That’s _tiny_ compared to full TIFF.

* * *

🔍 Existing libraries: what fits (and what doesn’t)
===================================================

❌ TinyTIFF
----------

*   No compression support  
    → **doesn’t help you at all**

* * *

⚠️ TinyDNG
----------

*   Has TIFF parsing
*   Some compression support

But:

*   Not focused on CCITT
*   Pulls in extra complexity (DNG, floating point, etc.)

👉 You’d spend time _removing_ things

* * *

⚠️ libtiff
----------

This is the only library that:

*   fully supports CCITT Group 4
*   is mature
*   battle-tested

But:

*   very large
*   historically security-sensitive
*   not designed for “minimal embed”

👉 You _can_ strip it down, but:

*   it’s not trivial
*   you still inherit complexity

* * *

🧠 The honest answer
====================

For your use case, the best options are:

* * *

🥇 Option A — Write a tiny custom decoder (recommended)
-------------------------------------------------------

This is actually more reasonable than it sounds.

Why?

Because your subset is so constrained:

*   no tiling
*   no multiple strips
*   no color formats
*   no metadata complexity
*   no multiple images

You basically need:

### 1\. Minimal TIFF parser (~300–800 LOC)

*   Read header (endianness)
*   Parse IFD
*   Extract a handful of tags
*   Validate strict constraints

* * *

### 2\. CCITT Group 4 decoder (~500–1500 LOC)

*   Bit-level decoding
*   Run-length decoding
*   2D encoding logic

👉 This is the _hardest technical part_, but:

*   well documented (fax standard)
*   many small implementations exist to reference

* * *

### 3\. Output → bitmap

*   1-bit → expand to 8-bit grayscale or RGBA for Skia

* * *

### Why this is actually viable

Compared to libtiff:

| Approach | Complexity |
| --- | --- |
| Full libtiff | massive |
| Stripped libtiff | still large |
| Your subset | **small and auditable** |

👉 And crucially:

*   **much easier to secure**
*   **much easier to fuzz**

* * *

🥈 Option B — Use libtiff but aggressively restrict it
------------------------------------------------------

If you don’t want to implement CCITT decoding yourself:

Use **libtiff**, but:

### Strategy:

*   Disable unused codecs at compile time
*   Only allow:
    *   `COMPRESSION_CCITTFAX4`
*   Reject:
    *   multiple strips
    *   multiple images
    *   unsupported tags

### Problems:

*   You still carry:
    *   a large codebase
    *   historical baggage
*   Harder to sandbox cleanly

👉 This is faster initially, slower long-term.

* * *

🥉 Option C — Hybrid approach (often best in practice)
------------------------------------------------------

*   Write your own **TIFF container parser**
*   Use a **small standalone CCITT decoder**

There are:

*   small fax decoders in:
    *   PDF engines
    *   embedded systems
    *   image libraries

👉 This gives you:

*   control over attack surface
*   reuse of tricky bit-level decoding

* * *

🔥 What I would actually do (if this were Chromium)
===================================================

1.  **Define strict acceptance rules**
    *   exactly one strip
    *   compression = Group 4
    *   1-bit only
    *   size limits (e.g. max pixels)
2.  Implement:
    *   tiny TIFF parser (custom)
3.  Either:
    *   port a small CCITT decoder  
        **OR**
    *   extract just that part from libtiff
4.  Reject everything else immediately

* * *

⏱️ Effort (revised for your narrow scope)
=========================================

With your constraints:

*   Parser: **~1–2 weeks**
*   CCITT decoder: **2–6 weeks**
*   Integration + testing: **3–6 weeks**

👉 Total: **~1–3 months**

Much smaller than the earlier estimate because you eliminated 90% of TIFF.

* * *

⚖️ Final takeaway
=================

*   There’s no perfect minimal off-the-shelf library for your exact niche
*   **libtiff is overkill** unless you heavily constrain it
*   Your use case is _so narrow_ that a custom implementation is actually the cleanest and safest route

* * *

If you want, I can sketch a **minimal TIFF header + IFD parser structure** or show exactly which tags you must validate to keep this airtight.

## Prompt:
to make this a bit more complex, i need TIFF image format support with two compression methods:

- JBIG2
- CCITT Group 4

actually JBIG2 has higher priority, because it has better compression

ok, lets do this...

where should i start?
should i write a TIFF parser from scratch, or should i teach tinytiff (https://github.com/jkriege2/TinyTIFF) to parse compressed images...?

## Response:
You’ve now moved from “simple TIFF subset” into something that is still **contained, but no longer trivial**—because JBIG2 changes the game.

A key correction up front:

> JBIG2 is not just a “compression option” inside TIFF in the same clean way CCITT is. It brings its own decoding ecosystem complexity.

That has architectural consequences.

* * *

🧠 First: your real problem decomposition
=========================================

You don’t actually need “TIFF support”.

You need:

> A **strict container parser + two bitstream decoders**

So your system is really:

1.  TIFF _container reader_
2.  Dispatcher (pick decoder based on compression tag)
3.  JBIG2 decoder
4.  CCITT Group 4 decoder
5.  Bitmap output → Skia

That’s it.

* * *

⚖️ TinyTIFF vs custom parser (important decision)
=================================================

🥇 Short answer
---------------

> You should **NOT try to extend TinyTIFF for this use case**.

You should instead:

> ✔️ Write your own minimal TIFF parser  
> ✔️ Plug in external decoders for JBIG2 + CCITT

* * *

🚫 Why TinyTIFF is a poor fit here
==================================

TinyTIFF is:

*   designed for **simple image I/O**
*   intentionally **does not model full TIFF semantics**
*   not structured as a pluggable decoder pipeline
*   not designed for:
    *   multiple compression backends
    *   strict validation rules
    *   security hardening

### The critical issue:

You would end up **fighting the abstraction layer**.

You’d likely have to:

*   bypass internal decoding
*   or fork heavily
*   or duplicate logic anyway

👉 At that point, you’ve lost the benefit of using it.

* * *

🏗️ Recommended architecture (clean + Chromium-friendly)
========================================================

✔️ 1. Write a minimal TIFF parser (you own this)
------------------------------------------------

Keep it _extremely strict_:

Supports only:

*   1 image per file
*   1 IFD only
*   baseline tags:
    *   ImageWidth
    *   ImageLength
    *   Compression
    *   StripOffsets
    *   StripByteCounts
    *   BitsPerSample = 1
    *   PhotometricInterpretation = 0 or 1

Reject everything else.

👉 This becomes a **small, auditable state machine**, not a full parser.

* * *

✔️ 2. Plug in external decoders
-------------------------------

### CCITT Group 4

You can either:

*   reuse a minimal fax decoder implementation
*   or extract just the relevant part from libtiff

This is relatively self-contained.

* * *

### JBIG2 (the big one)

JBIG2 is the real complexity driver.

You’ll likely use:

*   **jbig2dec** (common reference implementation)

Why:

*   widely used
*   designed as a standalone decoder
*   already used in PDF ecosystems

But note:

*   JBIG2 is _far more complex than CCITT_
*   includes:
    *   symbol dictionaries
    *   Huffman/arithmetic coding
    *   page composition logic

👉 This is the part that will dominate your integration effort.

* * *

🔥 Where the real work actually is
==================================

Now to your key question:

> “What will take the most time?”

🥇 1. JBIG2 integration (BIGGEST cost)
--------------------------------------

Not just decoding:

You must handle:

*   memory safety (JBIG2 has historically had CVEs)
*   bitmap composition
*   symbol reuse logic
*   page reconstruction

👉 This alone can take **weeks to months**

* * *

🥈 2. Security hardening of JBIG2 + TIFF boundary
-------------------------------------------------

Chromium-level requirements:

*   fuzz testing
*   heap limits
*   malformed stream handling
*   denial-of-service protection

👉 This is _mandatory_ and time-heavy

* * *

🥉 3. TIFF strict parser (surprisingly small)
---------------------------------------------

Because you are:

*   not supporting full TIFF
*   rejecting most of the spec

👉 This is actually the **easy part** (~1–2 weeks)

* * *

4\. Chromium integration (moderate)
-----------------------------------

*   Blink decoder hook
*   Skia bitmap conversion
*   sandboxing

* * *

🧭 Why NOT reuse TinyTIFF?
==========================

Because your design is:

> “TIFF is just a container header for compressed bitstreams”

But TinyTIFF is:

> “TIFF is an image decoding library”

Those are fundamentally different abstraction levels.

* * *

🧠 Best-practice architecture (what you should build)
=====================================================

### Step 1 — Minimal TIFF parser (custom)

*   no dependencies
*   strict validation only
*   returns:

undefined

* * *

### Step 2 — Decoder dispatch layer

undefined

* * *

### Step 3 — External decoders

*   JBIG2 → `jbig2dec`
*   CCITT → extracted minimal decoder

* * *

### Step 4 — Output normalization

*   convert to 1-bit or 8-bit grayscale
*   feed into Skia

* * *

⚖️ Final recommendation
=======================

### Do NOT:

*   try to extend TinyTIFF for compression
*   try to “teach” a general TIFF library your constraints

### Do:

*   write a **minimal TIFF container parser**
*   treat JBIG2 and CCITT as **separate codec modules**
*   keep everything strictly sandboxed

* * *

⏱️ Realistic effort (for your scoped design)
============================================

| Component | Time |
| --- | --- |
| TIFF minimal parser | 1–2 weeks |
| CCITT integration | 1–2 weeks |
| JBIG2 integration | 3–8+ weeks |
| Chromium hookup | 2–4 weeks |
| Security + fuzzing | ongoing / major |

👉 Total: **~2–4 months for a working prototype**, longer for Chromium-grade stability

* * *

If you want next step
=====================

I can outline:

*   a **minimal TIFF tag whitelist spec (exact fields + validation rules)**
*   or a **module diagram for Chromium integration**
*   or a **JBIG2 vs CCITT architectural comparison (important for design decisions)**

## Prompt:
oof, okay, so lets ignore JBIG2 for now, and focus on the TIFF parser, with support for CCITT-G4 compression, and future support for JBIG2 compression, but for now the "JBIG2 path" returns an error.

help me create such a TIFF parser, as a standalone C++ header-only library, suitable for later integration into the chromium codebase, but for now it is just a library, so i can use it in a CLI tool for testing.

can you create an initial project?
C++ or C or Rust or ...?
CMake or Meson or ...?
ideally with a CLI test tool
single header library?

## Response:
Good — this is now in a very sane “Chromium-shaped” design space: **strict TIFF subset + pluggable codec backend + CLI harness**.

I’ll steer you toward something that is:

*   small enough to reason about
*   easy to fuzz later
*   easy to drop into Chromium
*   not over-engineered

* * *

🧠 First: language choice (important)
=====================================

Best choice: **C++ (modern, minimal subset)**
---------------------------------------------

Why:

*   Chromium integration is C++
*   you’ll eventually need:
    *   memory control
    *   sandbox-friendly boundaries
    *   zero-copy buffers (important for images)
*   JBIG2 + CCITT libs are typically C/C++

👉 Rust would be fine, but:

*   harder Chromium integration later
*   FFI complexity shows up early

👉 Pure C:

*   simpler ABI, but more painful to structure cleanly

### Verdict:

> ✔️ Use **C++17 or C++20 (minimal STL)**

* * *

🏗️ Build system choice
=======================

Best: **CMake**
---------------

Why:

*   Chromium already uses it in parts
*   easiest cross-platform
*   easiest to integrate later as a `third_party/` module
*   works well with CLI tools + headers-only libs

* * *

📦 Architecture you want (clean + future-proof)
===============================================

We’ll design this as 3 layers:

undefined

* * *

🧱 Core design principles (VERY important for Chromium later)
=============================================================

Your parser MUST:

### ✔ Be strict

No tolerance for malformed TIFFs.

### ✔ Be allocation-bounded

Reject:

*   huge strips
*   absurd dimensions

### ✔ Avoid hidden dependencies

No libtiff, no boost, etc.

### ✔ Separate parsing from decoding

This is critical for JBIG2 later.

* * *

🧩 Minimal TIFF subset you will support
=======================================

We define a **hard whitelist**:

### Required tags:

*   ImageWidth (256)
*   ImageLength (257)
*   Compression (259)
*   StripOffsets (273)
*   StripByteCounts (279)
*   RowsPerStrip (278) optional (but typically 1 strip anyway)
*   PhotometricInterpretation (262)

### Constraints:

*   exactly **1 IFD**
*   exactly **1 strip**
*   BitsPerSample = 1
*   SamplesPerPixel = 1
*   planar config ignored or must be contiguous
*   byte order: little or big endian OK

* * *

🧠 Compression model
====================

We design a pluggable interface:

undefined

* * *

🧩 Core API design
==================

Main object
-----------

undefined

* * *

Decoder interface
-----------------

undefined

* * *

CCITT implementation hook
-------------------------

undefined

* * *

JBIG2 stub (future)
-------------------

undefined

* * *

🧱 TIFF parser (header-only core)
=================================

`include/tiff_min/tiff.hpp`
---------------------------

Key responsibilities:

*   parse header (II / MM)
*   read IFDs
*   extract tags
*   validate strict subset
*   dispatch decoder

* * *

Skeleton:
---------

undefined

* * *

🧠 Parsing strategy (important)
===============================

Inside `decode()`:

### Step 1: validate header

*   "II" or "MM"
*   42 magic number

### Step 2: read IFD offset

### Step 3: parse IFD entries into map

undefined

### Step 4: validate constraints

Example:

undefined

* * *

### Step 5: extract strip

*   read StripOffsets
*   read StripByteCounts
*   enforce:
    *   exactly 1 entry

* * *

### Step 6: dispatch decoder

undefined

* * *

🧪 CLI tool (important for testing)
===================================

`tools/tiff_cli.cpp`
--------------------

undefined

* * *

🧱 CMake structure
==================

undefined

* * *

🔥 What you get from this design
================================

### ✔ Chromium-ready structure

*   decoder abstraction matches Skia pipeline needs
*   clean separation of concerns

### ✔ Future JBIG2 support is trivial

*   just implement decoder and plug in

### ✔ Easy fuzzing target

*   single entry function:
    undefined

### ✔ No dependency lock-in

* * *

🧠 Critical design decision you made (correctly)
================================================

> “JBIG2 returns error for now”

This is VERY important because it:

*   keeps parser stable
*   avoids premature complexity
*   allows incremental integration

* * *

⚠️ One important warning
========================

Do NOT try to:

*   “be tolerant of broken TIFFs”
*   “support weird tags for compatibility”

For Chromium-style security:

> strict rejection is a feature, not a bug

* * *

🚀 If you want next step
========================

I can help you with:

### 1\. Full implementation of TIFF header parsing (byte-level safe code)

### 2\. CCITT G4 decoder integration strategy (or wrapper around existing minimal implementation)

### 3\. Fuzzing harness (VERY important for Chromium future)

### 4\. Chromium Blink integration path (how this plugs into `ImageDecoder`)

Just tell me which layer you want to build next.

## Prompt:
> Full implementation of TIFF header parsing (byte-level safe code)

yes

## Response:
Good — this is the right place to start. A **strict, byte-level TIFF parser** is the foundation of everything else, and if you get this wrong, every codec you plug in later will be fragile or insecure.

Below is a **minimal, Chromium-style, defensive TIFF header + IFD parser** written in modern C++ (header-only style).

It is designed to:

*   support only your constrained subset
*   avoid UB and unaligned reads
*   be fuzz-friendly
*   be easy to extend for CCITT G4 / JBIG2 later

* * *

🧱 Design goals (important)
===========================

This parser:

*   ❌ does NOT decode images
*   ❌ does NOT allocate large buffers blindly
*   ❌ does NOT support full TIFF spec
*   ✅ strictly validates structure
*   ✅ extracts only safe metadata
*   ✅ enforces “1-image / 1-strip” rule

* * *

📦 Minimal types
================

undefined

* * *

⚠️ Safe byte reader (core of everything)
========================================

This avoids:

*   unaligned memory access
*   buffer overruns
*   endian bugs

undefined

* * *

🧭 TIFF constants (minimal subset only)
=======================================

undefined

* * *

🧾 Parsed image metadata
========================

This is what your decoders will consume later:

undefined

* * *

🧠 Main parser
==============

undefined

* * *

🧠 What this already gives you
==============================

This parser enforces:

### ✔ Security boundaries

*   no OOB reads
*   strict tag rejection
*   strict size limits (implicitly via single-strip model)

### ✔ Chromium-aligned philosophy

*   fail-fast
*   no undefined behavior
*   no hidden allocations

### ✔ Clean handoff to decoders

You now have:

undefined

→ ready for:

*   CCITT G4 decoder
*   JBIG2 decoder (later stubbed)

* * *

⚠️ Important limitations (intentional)
======================================

This version intentionally does NOT support:

*   multiple IFDs
*   tiled TIFFs
*   per-tag type decoding (everything treated as inline uint32)
*   endian conversion correctness (we simplified; can be hardened next step)

These are deliberate for your constrained format.

* * *

🚀 Next step (recommended)
==========================

Now that parsing is done, the _next critical layer_ is:

👉 Safe strip extraction + decoder interface
--------------------------------------------

Specifically:

*   zero-copy view into compressed strip
*   decoder dispatch (CCITT vs JBIG2 stub)
*   output buffer strategy (1-bit → byte-expanded)

If you want, I can next build:

> 🧩 A CCITT G4 decoding interface + integration layer that plugs directly into this parser (Chromium-style, sandbox-friendly)



---
Powered by [ChatGPT Exporter](https://www.chatgptexporter.com)