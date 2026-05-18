# TestMem

Benchmark simplu pentru citirea unui fisier mare cu trei metode in C++: `mmap`, `ifstream` pe bucati si `ifstream` byte-cu-byte. Include un generator de CSV mare pentru teste.

## Continut

- `bigCSVgenerator.cpp`: genereaza un CSV mare (implicit 10 GiB) numit `financial_data.csv`.
- `MemMap/main.cpp`: citire cu `mmap` pe Linux.
- `MemMap/main_windows.cpp`: citire cu `mmap` Win32 pe Windows.
- `normal/chunked_ifstream.cpp`: citire cu buffer (8 MiB) folosind `ifstream`.
- `normal/simple_ifstream.cpp`: citire byte-cu-byte cu `ifstream`.
- `results.txt`: rezultate de benchmark (exemplu).

## Cerinte

- Linux: g++/clang++ cu suport C++17.
- Windows: Visual Studio (MSVC) sau g++/clang++ MinGW cu Win32 API.

## Generare fisier test

```bash
g++ -O2 -std=c++17 bigCSVgenerator.cpp -o bigCSVgenerator
./bigCSVgenerator
```

Generatorul scrie `financial_data.csv` in directorul curent. Modifica `TARGET_GB` in sursa daca vrei alta dimensiune.

## Build si rulare (Linux)

```bash
g++ -O2 -std=c++17 MemMap/main.cpp -o mmap_read

g++ -O2 -std=c++17 normal/chunked_ifstream.cpp -o ifstream_chunked

g++ -O2 -std=c++17 normal/simple_ifstream.cpp -o ifstream_simple
```

Rulare:

```bash
./mmap_read financial_data.csv 5
./ifstream_chunked financial_data.csv 5
./ifstream_simple financial_data.csv 5
```

Argumentul `iterations` este optional si trebuie sa fie > 0.

## Build si rulare (Windows)

Compilare (MSVC Developer Prompt):

```bat
cl /O2 /std:c++17 MemMap\main_windows.cpp /Fe:mmap_read.exe
cl /O2 /std:c++17 normal\chunked_ifstream.cpp /Fe:ifstream_chunked.exe
cl /O2 /std:c++17 normal\simple_ifstream.cpp /Fe:ifstream_simple.exe
```

Rulare:

```bat
mmap_read.exe financial_data.csv 5
ifstream_chunked.exe financial_data.csv 5
ifstream_simple.exe financial_data.csv 5
```

## Output

Fiecare metoda afiseaza:

- metoda folosita
- marimea fisierului (GiB)
- iteratii
- timp mediu per iteratie
- viteza (GiB/s)
- checksum (pentru validare)

## Note

- Rezultatele depind puternic de cache-ul de disc si de hardware.
- Pentru comparatie corecta, foloseste acelasi fisier si acelasi numar de iteratii.

## Rezultate

Vezi [results.txt](results.txt) pentru un exemplu de rulare.
