#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Parse a serial log produced by the on-device `spectrum` command and plot the
frequency spectrum.

The device prints lines prefixed with `[SPEC]`:

    [SPEC] META rate=48000 fft_size=1024 nbins=513 bands=10
    [SPEC] FREQ 31,63,125,250,500,1000,2000,4000,8000,16000
    [SPEC] FRAME 0 BAND_DB -62.10,-58.44,-49.02,...
    [SPEC] FRAME 1 BAND_DB -61.88,-57.90,-48.71,...
    [SPEC] END

This script extracts those lines and draws:
  1. a bar chart of the per-band power (averaged over all frames, or a single
     frame via --frame), and
  2. a spectrogram heat-map (bands on Y, frame index on X) when there is more
     than one frame.

Usage:
    python3 parse_spectrum.py log.txt
    python3 parse_spectrum.py log.txt --frame 3
    python3 parse_spectrum.py log.txt --save spectrum.png --no-show
"""

import argparse
import importlib
import re
import subprocess
import sys


def ensure_packages(packages):
    """Import each package, pip-installing any that are missing.

    `packages` maps the import name -> pip install name (usually identical).
    Returns a dict of import name -> imported module.
    """
    mods = {}
    missing = []
    for import_name in packages:
        try:
            mods[import_name] = importlib.import_module(import_name)
        except ImportError:
            missing.append(import_name)

    if missing:
        pip_names = [packages[name] for name in missing]
        print("missing packages: %s -- installing via pip..." % ", ".join(pip_names))
        try:
            subprocess.check_call(
                [sys.executable, "-m", "pip", "install", *pip_names]
            )
        except (subprocess.CalledProcessError, OSError) as e:
            sys.exit(
                "auto-install failed (%s).\n"
                "Please install manually: %s -m pip install %s"
                % (e, sys.executable, " ".join(pip_names))
            )
        # Refresh the import machinery so the freshly installed packages are found.
        importlib.invalidate_caches()
        for import_name in missing:
            try:
                mods[import_name] = importlib.import_module(import_name)
            except ImportError as e:
                sys.exit(
                    "still cannot import '%s' after install: %s" % (import_name, e)
                )

    return mods

META_RE = re.compile(
    r"\[SPEC\]\s+META\s+rate=(\d+)\s+fft_size=(\d+)\s+nbins=(\d+)\s+bands=(\d+)"
)
FREQ_RE = re.compile(r"\[SPEC\]\s+FREQ\s+([0-9,]+)")
FRAME_RE = re.compile(r"\[SPEC\]\s+FRAME\s+(\d+)\s+BAND_DB\s+([-0-9.,eE]+)")


def parse_log(path):
    """Return (meta, freqs, frames) parsed from a device log file."""
    meta = {}
    freqs = None
    frames = []  # list of (index, [float db, ...])

    with open(path, "r", errors="replace") as fh:
        for line in fh:
            m = META_RE.search(line)
            if m:
                meta = {
                    "rate": int(m.group(1)),
                    "fft_size": int(m.group(2)),
                    "nbins": int(m.group(3)),
                    "bands": int(m.group(4)),
                }
                continue

            m = FREQ_RE.search(line)
            if m:
                freqs = [int(x) for x in m.group(1).split(",") if x != ""]
                continue

            m = FRAME_RE.search(line)
            if m:
                idx = int(m.group(1))
                vals = [float(x) for x in m.group(2).split(",") if x != ""]
                frames.append((idx, vals))

    return meta, freqs, frames


def band_labels(freqs):
    """Human-readable x labels from center frequencies in Hz."""
    labels = []
    for f in freqs:
        if f >= 1000:
            labels.append(f"{f / 1000:g}k")
        else:
            labels.append(str(f))
    return labels


def main():
    ap = argparse.ArgumentParser(description="Parse and plot device spectrum logs.")
    ap.add_argument("logfile", help="serial log file containing [SPEC] lines")
    ap.add_argument("--frame", type=int, default=None,
                    help="plot this single frame index instead of the aggregate")
    ap.add_argument("--agg", choices=["mean", "max", "median"], default="mean",
                    help="how to combine frames when --frame is not given: "
                         "mean=average power in the linear domain (accurate default), "
                         "max=peak hold, median=robust to outlier frames (default: mean)")
    ap.add_argument("--save", default=None, help="save the figure to this file (e.g. out.png)")
    ap.add_argument("--no-show", action="store_true", help="do not open a window")
    args = ap.parse_args()

    meta, freqs, frames = parse_log(args.logfile)

    if not frames:
        sys.exit("No '[SPEC] FRAME ... BAND_DB' lines found in %s" % args.logfile)

    nbands = len(frames[0][1])
    if freqs is None or len(freqs) != nbands:
        # Fall back to indices if the FREQ line was missing / mismatched.
        freqs = list(range(nbands))
        print("warning: no matching [SPEC] FREQ line; using band indices", file=sys.stderr)

    # Drop malformed frames that don't have the expected band count.
    good = [(i, v) for (i, v) in frames if len(v) == nbands]
    if len(good) != len(frames):
        print("warning: dropped %d malformed frame(s)" % (len(frames) - len(good)),
              file=sys.stderr)
    frames = good

    labels = band_labels(freqs)

    # Lazy dependency handling so --help works without matplotlib installed,
    # and auto-install them the first time the script is actually used to plot.
    mods = ensure_packages({"numpy": "numpy", "matplotlib": "matplotlib"})
    np = mods["numpy"]
    matplotlib = mods["matplotlib"]
    if args.no_show:
        matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    mat = np.array([v for (_, v) in frames])  # shape: (n_frames, n_bands)

    if args.frame is not None:
        sel = [v for (i, v) in frames if i == args.frame]
        if not sel:
            sys.exit("frame %d not present in log" % args.frame)
        bar_vals = np.array(sel[0])
        bar_title = "Band power - frame %d" % args.frame
    elif args.agg == "max":
        # peak hold -- shows the strongest level each band ever reached
        bar_vals = mat.max(axis=0)
        bar_title = "Band power - peak of %d frames" % len(frames)
    elif args.agg == "median":
        # median in dB == median in linear (order-preserving), robust to outliers
        bar_vals = np.median(mat, axis=0)
        bar_title = "Band power - median of %d frames" % len(frames)
    else:
        # dB is logarithmic, so averaging dB values directly (mat.mean) underweights
        # the peaks: a strong 1000 Hz tone at -3.7 dB gets dragged down to ~-10 dB by
        # quieter/low-energy frames. Average in the LINEAR power domain and convert
        # back -- this is the physically correct mean power and matches the level a
        # steady tone actually holds.
        lin = np.power(10.0, mat / 10.0)          # dB -> linear power
        bar_vals = 10.0 * np.log10(lin.mean(axis=0) + 1e-20)
        bar_title = "Band power - mean power of %d frames" % len(frames)

    rate = meta.get("rate", "?")
    fft = meta.get("fft_size", "?")

    has_spectrogram = mat.shape[0] > 1
    nplots = 2 if has_spectrogram else 1
    fig, axes = plt.subplots(nplots, 1, figsize=(10, 4 * nplots))
    if nplots == 1:
        axes = [axes]

    # --- bar chart ---
    ax = axes[0]
    ax.bar(range(nbands), bar_vals, color="#3b7dd8")
    ax.set_xticks(range(nbands))
    ax.set_xticklabels(labels, rotation=45, ha="right")
    ax.set_xlabel("Frequency (Hz)")
    ax.set_ylabel("Power (dB)")
    ax.set_title("%s   [rate=%s, fft=%s]" % (bar_title, rate, fft))
    ax.grid(True, axis="y", linestyle=":", alpha=0.5)

    # --- spectrogram ---
    if has_spectrogram:
        ax = axes[1]
        im = ax.imshow(mat.T, aspect="auto", origin="lower",
                       interpolation="nearest", cmap="magma")
        ax.set_yticks(range(nbands))
        ax.set_yticklabels(labels)
        ax.set_xlabel("Frame index")
        ax.set_ylabel("Frequency (Hz)")
        ax.set_title("Spectrogram (%d frames)" % mat.shape[0])
        fig.colorbar(im, ax=ax, label="Power (dB)")

    fig.tight_layout()

    if args.save:
        fig.savefig(args.save, dpi=120)
        print("saved figure to %s" % args.save)

    if not args.no_show:
        plt.show()


if __name__ == "__main__":
    main()
