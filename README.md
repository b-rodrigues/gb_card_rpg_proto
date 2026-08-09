# Nix Game Boy LLM Development Kit

A minimal, deterministic, Nix-based development environment for creating Nintendo Game Boy games.

## Requirements

Nix with flakes enabled.

## Setup

```bash
nix develop
```

## Build

```bash
make
```

## Run

```bash
make run
```

## Test

```bash
make test
```

## Screenshot

```bash
make screenshot
```

## ROM

```text
build/game.gb
```

## Controls

| Button | Action |
| --- | --- |
| **Up** | Hard drop piece (instantly drop to bottom) |
| **Down** | Soft drop piece |
| **Left** | Move piece left |
| **Right** | Move piece right |
| **A** | Rotate piece clockwise |
| **B** | Rotate piece counter-clockwise |
| **Select** | Hold / swap Tetromino (once per piece drop) / Restart on Pause |
| **Start** | Pause / Resume game |
