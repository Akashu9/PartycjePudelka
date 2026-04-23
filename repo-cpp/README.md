# Partycje Prostopadloscianu - wersja C++ (jedno okno)

Aplikacja desktopowa C++/Qt6 uruchamiana jako jedno samodzielne okno.

## Funkcje

- liczenie unikalnych partycji prostopadloscianu `m x n x k`,
- deduplikacja do multizbiorow wymiarow,
- jedno przykladowe ulozenie dla kazdej partycji,
- wizualizacja 3D: obrot, zoom, przezroczystosc, przekroje `X <= t`, `Y <= t`, `Z <= t`,
- tryb tylko zliczania,
- limit czasu i reczne przerwanie obliczen.

## Wymagania

- CMake 3.21+
- kompilator C++20
- Qt6 (`Widgets`)

## Budowanie i uruchamianie (Linux/macOS)

```bash
cmake -S . -B build
cmake --build build -j
./build/partycje_pudelka_cpp
```

## Budowanie i uruchamianie (Windows)

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\partycje_pudelka_cpp.exe
```

## Walidacja

Dla `m=2`, `n=2`, `k=2` wynik powinien byc: **10 partycji**.
