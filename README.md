# Partycje Prostopadloscianu - wersja C++ (jedno okno)

Projekt zostal przepisany na aplikacje desktopowa C++/Qt6 uruchamiana jako jedno samodzielne okno.

## Co robi aplikacja

- liczy unikalne partycje prostopadloscianu `m x n x k` na osiowo rownolegle klocki,
- deduplikuje wyniki do multizbiorow wymiarow (ignoruje polozenie geometryczne elementow),
- przechowuje jedno przykladowe ulozenie dla kazdej partycji,
- rysuje wizualizacje 3D z obrotem, zoomem, przezroczystoscia i przekrojami `X <= t`, `Y <= t`, `Z <= t`,
- umozliwia tryb "tylko zliczanie" bez zapisu geometrii,
- obsluguje limit czasu i reczne przerwanie obliczen.

## Wymagania

- CMake 3.21+
- kompilator C++20
- Qt6 (modul `Widgets`)

## Budowanie i uruchamianie (Linux/macOS)

```bash
cd cpp
cmake -S . -B build
cmake --build build -j
./build/partycje_pudelka_cpp
```

## Budowanie i uruchamianie (Windows, PowerShell)

```powershell
cd cpp
cmake -S . -B build
cmake --build build --config Release
.\build\Release\partycje_pudelka_cpp.exe
```

## Szybka walidacja

Dla `m=2`, `n=2`, `k=2` aplikacja powinna zwrocic **10 partycji** (zgodnie z materialami w `specyfikacja/`).

## Struktura C++

- `cpp/src/PartitionSolver.*` - solver DFS + deduplikacja partycji
- `cpp/src/RenderWidget.*` - render 3D i obsluga interakcji mysza
- `cpp/src/MainWindow.*` - UI, sterowanie obliczeniami, lista wynikow
- `cpp/src/main.cpp` - punkt startowy aplikacji
