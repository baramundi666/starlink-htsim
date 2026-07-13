# Raport z realizacji projektu

## Analiza dostępności, stabilności topologii oraz wpływu zagęszczenia konstelacji satelitarnej na parametry sieci

### Autorzy

- Iga Antonik
- Mateusz Król
- Łukasz Wilański


## 1. Cel projektu

Celem projektu było przeprowadzenie analizy działania symulowanej konstelacji satelitów na niskiej orbicie okołoziemskiej (LEO) z wykorzystaniem środowiska **starlink-htsim**.

Badania skupiały się na trzech głównych aspektach:

- zbadaniu dostępności oraz stabilności topologii sieci,
- określeniu wpływu zagęszczenia konstelacji na parametry transmisji,
- weryfikacji poprawności działania zaimplementowanego modelu oraz algorytmów routingu.

Realizacja projektu została przedstawiona zgodnie z metodologią **OKR (Objectives and Key Results)**.


## 2. Środowisko badawcze

Badania przeprowadzono z wykorzystaniem symulatora **starlink-htsim**, bazującego na zmodyfikowanym środowisku **htsim**.

Najważniejsze elementy środowiska:

- model konstelacji satelitów LEO,
- model ruchu orbitalnego,
- dynamiczne połączenia ISL (Inter Satellite Links),
- węzły naziemne,
- algorytm Dijkstry odpowiedzialny za wyznaczanie tras,
- automatyczna analiza wyników do plików CSV.


## 3. Metodyka badań

Przeprowadzono pięć scenariuszy eksperymentalnych:

| Scenariusz | Cel |
|---|---|
| A | Walidacja poprawności działania (Sanity Test) |
| B | Małe konfiguracje testowe |
| C | Wpływ zagęszczenia konstelacji |
| D | Analiza ruchu sieciowego i kolejek |
| E | Wpływ parametrów łączy ISL |

Analizowane metryki:

- availability_pct
- mean_rtt_ms
- p95_rtt_ms
- mean_isl_hops
- route_changes_per_min
- mean_route_segment_duration_s
- mean_dijkstra_cpu_ms


## 4. Objectives and Key Results (OKR)


### Objective 1 - Zbadanie dostępności oraz stabilności topologii sieci satelitarnej

#### Key Results

- KR1 — wyznaczenie procentowej dostępności tras
- KR2 — analiza zmian tras w czasie


#### Wyniki

##### Dostępność tras

![](figures/01_availability_by_case.png)

**Rysunek 1.** Dostępność tras dla poszczególnych scenariuszy.

**Opis wyników**

Zgodnie z założeniami modelu fizycznego, konstelacje o odpowiedniej gęstości węzłów (scenariusze C, D, E) gwarantują stuprocentową, nieprzerwaną dostępność tras. Z kolei w drastycznie zredukowanych, minimalistycznych topologiach (scenariusze A, B) symulator poprawnie odnotowuje występujące okresowo luki w pokryciu sygnałem, co skutkuje brakiem ścieżki routingu (stan `NO_ROUTE`).

---

##### Stabilność tras

![](figures/03_route_changes_per_min.png)

**Rysunek 2.** Liczba zmian tras na minutę.

**Opis wyników**

Analiza wykazuje, że pełne zagęszczenie konstelacji (24 płaszczyzny) prowadzi do drastycznego spadku stabilności samej topologii ścieżki. W zoptymalizowanej sieci trasa potrafi zmieniać się od 3 do nawet 4 razy w ciągu zaledwie jednej minuty, co jest wynikiem stałego poszukiwania najkrótszej drogi przez algorytm Dijkstry przy szybko poruszających się węzłach orbitalnych. Rzadsze konstelacje wymuszają trzymanie się dłuższych i mniej optymalnych, ale za to rzadziej rekonfigurowanych ścieżek.


#### Podsumowanie Objective 1

Na podstawie przeprowadzonych badań określono poziom dostępności tras oraz stabilność topologii dla wszystkich analizowanych konfiguracji konstelacji.

### Objective 2 - Zbadanie wpływu zagęszczenia konstelacji na parametry sieci

#### Key Results

- KR1 — porównanie konfiguracji 6, 12 i 24 płaszczyzn
- KR2 — analiza RTT
- KR3 — analiza liczby hopów ISL


#### Wyniki

##### Średnie RTT

![](figures/02_mean_rtt_by_case.png)

**Rysunek 3.** Średnie opóźnienia RTT.

**Opis wyników**

Zagęszczenie infrastruktury sieciowej bezpośrednio redukuje czas transmisji. W pełnych konfiguracjach sygnał podróżuje najbardziej optymalną drogą, podczas gdy redukcja pokrycia orbitalnego zmusza algorytm do wybierania tras suboptymalnych, co widocznie zwiększa średnie opóźnienie RTT na badanych dystansach.

---

##### Liczba hopów ISL

![](figures/05_mean_isl_hops.png)

**Rysunek 4.** Średnia liczba hopów.

**Opis wyników**

Zmniejszenie liczby płaszczyzn (np. z 24 do zaledwie 6) znacząco wydłuża logiczną długość trasy w grafie. Prowadzi to do powstania łączy okrężnych, które wymagają wielokrotnego przekazywania pakietu. Każdy dodatkowy węzeł w ścieżce (hop) dodaje niepożądane opóźnienie propagacyjne oraz czas niezbędny na wewnątrzsatelitarne przetwarzanie danych.

---

##### RTT dla różnych zagęszczeń

![](figures/06_C_NY_Seattle_rtt_timeseries.png)

**Rysunek 5.** RTT w czasie.

**Opis wyników**

Dla konstelacji zredukowanej (6 płaszczyzn) wartość RTT jest bardzo niestabilna, charakteryzując się nagłymi skokami w funkcji czasu. Pełne rozwinięcie konstelacji (24 płaszczyzny) nie tylko obniża opóźnienie do poziomu około 31 ms, ale również drastycznie wygładza wykres, co zapewnia pożądaną, wysoką przewidywalność dostarczania pakietów.

---

##### Rozkład RTT

![](figures/07_C_NY_Seattle_rtt_boxplot.png)

**Rysunek 6.** Boxplot RTT.

**Opis wyników**

Analiza statystyczna zjawiska potwierdza ogromną wariancję czasów transmisji w przerzedzonej sieci. Większa liczba aktywnych płaszczyzn zawęża rozstęp kwartylny do minimum, redukując ekstrema i sprawiając, że większość mierzonych wartości RTT zbiega do optymalnego minimum opóźnienia propagacyjnego.


#### Podsumowanie Objective 2

Przeprowadzone eksperymenty wykazały zależność pomiędzy zagęszczeniem konstelacji a parametrami transmisji. Wraz ze wzrostem liczby satelitów zmniejszały się opóźnienia oraz liczba przeskoków pomiędzy satelitami.


### Objective 3 - Weryfikacja poprawności działania modelu

#### Key Results

- KR1 — poprawne działanie scenariusza Sanity
- KR2 — poprawne działanie algorytmu Dijkstry
- KR3 — poprawna obsługa zmian topologii


#### Wyniki

##### Walidacja scenariusza testowego

**Opis wyników**

Scenariusze sprawdzające logikę (`sanity`, `small scale`) poprawnie zamodelowały sytuacje zrywania łączności radiowej, do których dochodziło, gdy satelity fizycznie znikały za horyzontem miast. Geometria sferyczna Ziemi została należycie ujęta w symulacji widoczności sprzętu po obydwu stronach (Uplink/Downlink).

---

##### Koszt działania algorytmu

![](figures/04_mean_dijkstra_cpu_ms.png)

**Rysunek 7.** Średni czas działania algorytmu Dijkstry.

**Opis wyników**

Obliczenie najkrótszej ścieżki w tak złożonym, dynamicznym układzie wiąże się ze znacznym narzutem procesora. W przypadku symulacji uwzględniającej 24 płaszczyzny średni koszt jednorazowego przeliczenia tras wzrasta do ponad 30 ms. Weryfikuje to poprawność przeliczania całościowego grafu powiązań między węzłami i obnaża wyzwania ze skalowalnością algorytmu.

---

##### Analiza ruchu sieciowego

![](figures/08_D_London_NY_ping_rtt_timeseries.png)

**Rysunek 8.** RTT podczas aktywnego ruchu sieciowego.

**Opis wyników**

Po włączeniu symulacji ruchu pakietowego zaobserwowano ekstremalne, krótkotrwałe skoki opóźnienia (jitter). Jest to odzwierciedleniem zjawiska opóźnień kolejkowania (ang. *queuing delay*). Skoki występują w momencie, gdy pakiety pomiarowe trafiają do satelitów charakteryzujących się dużym obciążeniem buforów pamięci i konkurują o wąskie gardło z pozostałym ruchem generowanym na tym samym połączeniu.

#### Podsumowanie Objective 3

Przeprowadzone testy potwierdziły poprawność działania modelu symulacyjnego oraz implementacji algorytmów routingu we wszystkich analizowanych scenariuszach.


## 5. Podsumowanie realizacji OKR

| Objective | Status |
|---|---|
| Zbadanie dostępności i stabilności topologii |  Zrealizowano |
| Zbadanie wpływu zagęszczenia konstelacji | Zrealizowano |
| Weryfikacja poprawności działania modelu |  Zrealizowano |

Najważniejsze obserwacje:

- gęstsza konstelacja zapewnia większą dostępność tras,
- zwiększenie liczby satelitów zmniejsza opóźnienia RTT,
- liczba hopów ISL maleje wraz ze wzrostem zagęszczenia,
- model poprawnie odwzorowuje dynamiczne zmiany topologii,
- implementacja algorytmu Dijkstry działa zgodnie z oczekiwaniami, ale generuje narzut CPU.


## 6. Ograniczenia projektu

Najważniejsze ograniczenia modelu:

- brak naziemnych stacji przekaźnikowych (Relay Stations), co może zawyżać czas RTT w niektórych scenariuszach,
- uproszczony model propagacji sygnału,
- ograniczona liczba scenariuszy ruchowych (brak uwzględnienia skomplikowanych protokołów congestion control),
- routing oparty wyłącznie o scentralizowany algorytm Dijkstry i nieuwzględniający rozwiązań rozproszonych.


## 7. Wnioski

Zrealizowane eksperymenty pozwoliły osiągnąć wszystkie założone cele projektu. Analiza wykazała, że zagęszczenie konstelacji ma bezpośredni wpływ na dostępność tras, stabilność topologii oraz opóźnienia transmisji. Zidentyfikowano również wyzwania wynikające z opóźnień związanych z kolejkowaniem ruchu sieciowego w satelitach. Wyniki potwierdziły poprawność działania implementacji oraz przydatność środowiska **starlink-htsim** do analizy sieci satelitarnych LEO.