# Paramedic EQ v2.0.5 — Dvinesoul Edition

This package is the rebuilt Paramedic EQ interface based on the supplied reference design.

## Included

- 29-band parametric EQ
- Gold/black metallic interface
- Preset navigation and preset list
- Input/output sections
- EQ curve and animated spectrum display
- 29-band control matrix
- Band Select panel
- Reverb Engine controls
- Analyzer controls
- Presets panel
- Master Section
- Compressor / Limiter UI
- Power control
- Dvinesoul branding
- Source-derived X-Core logo asset, recoloured gold for the Paramedic EQ theme

## Important build fix

The previous v2.0.2 source was missing the JUCE plugin factory function. The v2.0.3 source includes:

    juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
    {
        return new ParamedicEQAudioProcessor();
    }

## Build on Kali / Linux

From this directory:

    rm -rf build
    cmake -B build -G Ninja
    cmake --build build --config Release

The build uses JUCE 8.0.13 through CMake FetchContent.

The generated VST3 is copied to:

    ~/.vst3/Paramedic EQ.vst3

The standalone application is generated under:

    build/ParamedicEQ_artefacts/Standalone/

## Reference

The supplied reference image is treated as the visual layout target. The GUI is fixed at 1536x1020 to prevent proportional drift between the EQ graph, 29-band matrix, and lower panels.


## Graph band dragging

The numbered EQ nodes in the main graph are now interactive. Click and drag any numbered band node:

- Drag left/right to change frequency (20 Hz–20 kHz, logarithmic scale).
- Drag up/down to change gain (-24 dB to +18 dB).
- The selected band follows the node while dragging.
- The existing frequency and gain controls are updated through the JUCE parameter attachments, so preset saving/loading also preserves the changes.
- Q remains controlled by its existing knob.

This is intentionally limited to the numbered EQ nodes so the rest of the reference GUI remains unchanged.


## User Library

Paramedic EQ uses a dedicated, easy-to-access user library.

On Windows:
`%USERPROFILE%\\Documents\\Paramedic EQ\\Library\\`

Presets:
`%USERPROFILE%\\Documents\\Paramedic EQ\\Library\\Presets\\`

The plugin creates the Presets directory automatically when saving or opening a preset.
