# Changelog

What changed between the published versions of Trioio. The development
notes, in Portuguese, are in [`DESENVOLVIMENTO.md`](DESENVOLVIMENTO.md) and
[`TRIOIO_TAREFAS.md`](TRIOIO_TAREFAS.md).

## Unreleased

- **Streaming version at −14 LUFS** (saida-ref 1.1, the family's common
  technical output): next to each response, `_streaming.wav` (−14 LUFS, true
  peak ≤ −1 dBTP, 24-bit PCM with TPDF) and a `.txt` with the measurements,
  made offline on a thread of its own. The live sound and the raw recording
  do not change. Dynamics guard: the limiter never takes more than 6 dB; a
  take too dynamic for that stays under −14 and the `.txt` says why.
  The response itself stays on Trioio's own master (−16 LUFS).
- **TPDF dither** on the response, the lines and the stems (24-bit; JUCE
  used to truncate).
- Output conformance tests: integrated loudness (EBU Tech 3341 1–5), true
  peak (15–23), the master is bit-exact for the same input.

## v0.2.0 — 2026-11-16

- **Maestruc receiver** (the RASGO family's conductor, protocol
  `maestruc.link/1`, local OSC only): with `--maestruc` or
  `TRIOIO_MAESTRUC=1`, Maestruc can play the chosen response (the same ▶)
  and stop, and know whether Trioio is playing. Composing and analysing stay
  outside: they depend on the declarer's authorisation. Off by default.
  Copy of `MAESTRUC/receptor/MaestrucLink.h`; `juce_osc` module in the app.
- **Sound bridge to Maestruc** (Windows, macOS and Linux, no virtual audio
  driver): with `--maestruc`, the mix goes to Maestruc through a shared-memory
  file only the user can read; while Maestruc is reading, Trioio silences its
  own output and, if Maestruc closes or stops, plays on its own again within
  about half a second. Copy of `MAESTRUC/receptor/PonteDeSom.h`.
- **Master from Maestruc:** the same master volume as the window (−60 dB,
  silent, to +6 dB), and mute (to −60 dB and back to the level before);
  Trioio has no per-channel mixer. Commands are only
  accepted with the session password Maestruc writes to a file only the user
  can read.
- **Library: "hide refused (N)".** A box in the Library hides the refused
  references from the list. Nothing is deleted: the record of each
  reference stays (provenance is never erased), and unticking shows them
  again. The choice is remembered. The box appears only when there is a
  refused reference.

## v0.1.1 — 2026-10-19 (preview)

- **The "file ▾" button** next to the chosen response:
  - **show in folder**: opens the file manager on the response's files;
  - **export…**: copies the mix, the MIDI, the stems, the lines and
    `decisoes.json` to a folder you choose, into a subfolder named after the
    session. Nothing is overwritten;
  - **move to the trash…**: asks first, and moves the response to the
    `lixeira/` folder of your data folder, from where it can be brought
    back. A judgement of it stays in your listening.
- The tutorial (chapter 6) and the guide explain the button, in four
  languages.

## v0.1.0 — 2026-10-19 (preview)

First public version: Windows, macOS and Linux packages, a tutorial in four
languages and the installation guide.

## Mudanças (português)

- **v0.1.1:** o botão **arquivo ▾** junto da resposta, com três ações:
  - mostrar na pasta;
  - exportar (WAV, MIDI, stems, linhas e decisões para a pasta escolhida,
    sem sobrescrever nada);
  - mover para a lixeira, com confirmação e reversível.
- **v0.1.0:** a primeira versão pública.
