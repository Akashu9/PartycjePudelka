# Partycje Prostopadłościanu - aplikacja web

Aplikacja liczy unikalne partycje prostopadłościanu `m×n×k` na osiowo równoległe klocki o całkowitych dodatnich wymiarach.

## Funkcje

- pełna enumeracja dopuszczalnych rozkładów,
- deduplikacja do multizbiorów wymiarów (z pominięciem permutacji wymiarów klocka),
- jedno przykładowe ułożenie dla każdej partycji,
- wizualizacja 3D (`canvas`) z obrotem, zoomem, przezroczystością i przekrojem `X<=t`, `Y<=t`, `Z<=t`,
- tryb minimalny: samo zliczanie (bez zapisu geometrii).

## Uruchomienie

### Linux

Wejdź do katalogu projektu i uruchom skrypt:

```bash
./start-linux.sh
```

Opcjonalnie inny port:

```bash
./start-linux.sh 9000
```

### Windows

1. Uruchom:

```bat
start-windows.bat
```

Opcjonalnie otwórz `cmd` lub PowerShell i przejdź do katalogu projektu:

```bat
start-windows.bat 9000
```

### Ręcznie (alternatywnie)

Jeśli nie chcesz używać skryptów:

```bash
python -m http.server 8080
```

Potem otwórz:

```text
http://localhost:8080
```

## Pliki

- `index.html` - interfejs
- `style.css` - style
- `app.js` - logika UI i rysowanie 3D
- `solver-worker.js` - algorytm obliczeń w Web Workerze
- `start-linux.sh` - skrypt startowy dla Linux
- `start-windows.bat` - skrypt startowy dla Windows
