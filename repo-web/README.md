# WYMAGANY ZAINSTALOWANY PYTHON

Przetestowany na wersji 3.14

# Partycje Prostopadłościanu - aplikacja web

Aplikacja liczy unikalne partycje prostopadłościanu `m×n×k` na osiowo równoległe klocki o całkowitych dodatnich wymiarach.

## Funkcje

- pełna enumeracja dopuszczalnych rozkładów,
- deduplikacja do multizbiorów wymiarów (z pominięciem permutacji wymiarów klocka),
- jedno przykładowe ułożenie dla każdej partycji,
- wizualizacja 3D (`canvas`) z obrotem, zoomem, przezroczystością i przekrojem `X<=t`, `Y<=t`, `Z<=t`,
- tryb minimalny: samo zliczanie (bez zapisu geometrii).

## Uruchomienie

### Windows

1. Otwórz folder projektu.
2. Kliknij dwukrotnie plik `start-windows-easy.bat`.
3. Poczekaj, aż przeglądarka otworzy stronę `http://localhost:8080`.
4. Nie zamykaj czarnego okna skryptu podczas pracy aplikacji.

Jeśli pojawi się komunikat o braku Pythona:

1. Skrypt otworzy stronę pobierania Pythona.
2. Zainstaluj Python 3.
3. W instalatorze zaznacz opcję `Add python.exe to PATH`.
4. Uruchom ponownie `start-windows-easy.bat`.

Jak zatrzymać aplikację: zamknij okno skryptu.

### Windows (wersja z parametrem portu)

```bat
start-windows.bat
```

Opcjonalnie inny port:

```bat
start-windows.bat 9000
```

### Linux

Wejdź do katalogu projektu i uruchom skrypt:

```bash
./start-linux.sh
```

Opcjonalnie inny port:

```bash
./start-linux.sh 9000
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
- `start-windows.bat` - skrypt startowy dla Windows (z parametrem portu)
- `start-windows-easy.bat` - najprostszy start dla Windows (dwuklik)
