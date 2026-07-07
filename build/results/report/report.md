# Raport z analizy wydajności i protokołów routingu w konstelacjach satelitarnych (starlink-htsim)
### Autorzy
- Iga Antonik
- Mateusz Król
- Łukasz Wilański

Niniejszy dokument przedstawia wyniki badań nad wydajnością symulowanych sieci satelitarnych na niskiej orbicie okołoziemskiej, inspirowanych architekturą konstelacji Starlink. Wykorzystując środowisko symulacyjne oparte na zmodyfikowanym rdzeniu `htsim`, przeprowadziliśmy serię benchmarków. Badania miały na celu ewaluację dostępności tras, opóźnień (RTT).


## 1. Środowisko Symulacyjne i Architektura

Projekt opiera się na dyskretno-zdarzeniowym symulatorze sieciowym, rozbudowanym o moduł fizyki orbitalnej i geometrii sferycznej.

Symulator działa w oparciu o następujące komponenty główne:

* **Constellation:** Moduł odpowiedzialny za generowanie układu satelitów na podstawie zadanej liczby płaszczyzn orbitalnych oraz zagęszczenia węzłów w poszczególnych płaszczyznach. Obsługuje zarówno tryb `spread` (równomierne rozmieszczenie), jak i `adjacent` (symulacja sąsiadujących węzłów do celów testowych).
* **City:** Węzeł naziemny (endpoint) przeliczający swoje współrzędne w funkcji czasu na obracającej się Ziemi. Odpowiada za aktywację łączności bezprzewodowej (Uplink/Downlink) wyłącznie do satelitów znajdujących się w zasięgu geometrycznym.
* **Algorytm Dijkstry:** Moduł dynamicznego wyznaczania najkrótszych ścieżek grafowych przez łącza międzysatelitarne (ISL), aktualizujący trasy w zdefiniowanych interwałach (np. co 1000 ms).

Proces badawczy został zautomatyzowany poprzez skrypt (`run_benchmarks.sh`), który po zakończeniu symulacji wykorzystuje parser `parse_output.cpp` do ekstrakcji i agregacji danych z binarnych logów zdarzeniowych do ustrukturyzowanych plików CSV.


## 2. Metodologia i Scenariusze Badawcze (Benchmarks)

W celu zbadania zachowania sieci, wzięliśmy pod uwagę 5 scenariuszy:

* **A (`sanity`)**: Test minimalnej konfiguracji (1 płaszczyzna, 2 satelity w trybie `adjacent`), służący do walidacji podstawowej fizyki modelu.


* **B (`small scale`)**: Scenariusze testowe z rosnącą liczbą satelitów i płaszczyzn.


* **C (`partial deployment`)**: Symulacja połączenia Nowy Jork $\rightarrow$ Seattle dla konstelacji częściowych (6, 12, 24 płaszczyzn orbitalnych), badająca wpływ gęstości pokrycia na połączenie.


* **D (`ping & queue`)**: Połączenie Londyn $\rightarrow$ Nowy Jork z aktywnym ruchem sieciowym.


* **E (`ISL sensitivity`)**: Analiza wrażliwości opóźnień na zmiany bazowej przepustowości łączy międzysatelitarnych (ISL).



Poddano analizie szereg metryk, w tym: 
- `availability_pct` — procentowy wskaźnik poprawnie odnalezionych tras (procent próbek, w których `route_found == 1` dla kierunku `out`).
- `mean_rtt_ms`, `p95_rtt_ms` — RTT liczone tylko dla próbek, w których trasa istnieje.
- `route_changes_per_min` — liczba zmian trasy na minutę; obejmuje także przejścia do `NO_ROUTE`.
- `mean_route_segment_duration_s` — średni czas trwania spójnego segmentu tej samej znalezionej trasy.
- `mean_isl_hops` — średnia liczba hopów przez inter-satellite links.
- `mean_dijkstra_cpu_ms` — średni koszt CPU pojedynczego wyszukiwania trasy.


## 3. Analiza Wyników

### 3.1. Dostępność Tras i Stabilność Topologii

Krytycznym parametrem sieci jest utrzymanie fizycznej ciągłości ścieżki routingu.

<figure>
  <img src="figures/01_availability_by_case.png" alt="">
  <figcaption><b>Wykres 1.</b> Odsetek próbek z sukcesem wyznaczoną trasą dla poszczególnych scenariuszy badawczych</figcaption>
</figure>

Konstelacje z odpowiednią gęstością węzłów (C, D, E) gwarantują 100% dostępność trasy. W skrajnie zredukowanych topologiach (A, B) dochodzi do przerw w łączności (stan `NO_ROUTE`).


### 3.2. Wpływ Zagęszczenia Konstelacji na Opóźnienia (RTT)

Aby w pełni zrozumieć wpływ architektury sieci na opóźnienia, należy przeanalizować średnie wartości RTT dla wszystkich przeprowadzonych scenariuszy.

<figure>
  <img src="figures/02_mean_rtt_by_case.png" alt="">
  <figcaption><b>Wykres 2.</b> Zestawienie średniego opóźnienia (Mean RTT) we wszystkich badanych konfiguracjach (liczone wyłącznie dla pomyślnie wyznaczonych tras)</figcaption>
</figure>

<figure>
  <img src="figures/05_mean_isl_hops.png" alt="">
  <figcaption><b>Wykres 3.</b> Średnia liczba przeskoków międzysatelitarnych (ISL Hops) wymaganych do zestawienia połączenia</figcaption>
</figure>

Powyższe wykresy wykazują powiązanie między gęstością konstelacji a długością ścieżki logicznej. Zmniejszenie liczby płaszczyzn (z 24 do 6 w C) wymusza na algorytmie routingu wykorzystywanie łączy okrężnych. Przekłada się to bezpośrednio na wzrost średniej liczby przeskoków (ISL hops) między satelitami. Każdy dodatkowy węzeł pośredni w ścieżce (hop) dodaje opóźnienie propagacyjne oraz czas przetwarzania pakietu, co wprost tłumaczy drastyczny wzrost średniego opóźnienia (Mean RTT) w rzadszych topologiach.

Badania wpływu architektury (6, 12, 24 płaszczyzny) na opóźnienia zobrazowano na poniższych wykresach.

<figure>
  <img src="figures/06_C_NY_Seattle_rtt_timeseries.png" alt="">
  <figcaption><b>Wykres 4.</b> Opóźnienie RTT w funkcji czasu dla połączenia NY-Seattle w różnych wariantach pokrycia</figcaption>
</figure>

<figure>
  <img src="figures/07_C_NY_Seattle_rtt_boxplot.png" alt="">
  <figcaption><b>Wykres 5.</b> Rozkład statystyczny wartości RTT w zależności od liczby aktywnych płaszczyzn orbitalnych</figcaption>
</figure>


Gęstość konstelacji bezpośrednio koreluje ze średnim opóźnieniem. Dla zredukowanej konstelacji (6 płaszczyzn), algorytm Dijkstry jest zmuszony wyznaczać ścieżki suboptymalne, korzystając z większej liczby łączy ISL o wyższym koszcie propagacyjnym. Skutkuje to zarówno podwyższoną wartością `mean_rtt_ms` (ok. 45 ms), jak i ogromnym rozstrzałem opóźnień widocznym na wykresie. Pełna konstelacja (24 płaszczyzny) optymalizuje trasę, drastycznie zmniejszając RTT do ok. 31 ms i stabilizując opóźnienie w czasie.

### 3.3. Obciążenie Routingu i Zmiany Tras

Wyznaczenie nowej trasy w dynamicznym grafie generuje koszt w postaci zużycia CPU.

<figure>
  <img src="figures/03_route_changes_per_min.png" alt="">
  <figcaption><b>Wykres 6.</b> Liczba zmian ścieżek routingu na minutę</figcaption>
</figure>


<figure>
  <img src="figures/04_mean_dijkstra_cpu_ms.png" alt="">
  <figcaption><b>Wykres 7.</b> Średni koszt CPU (ms) jednorazowego przeliczenia tras w systemie</figcaption>
</figure>


Konstelacje o pełnym zagęszczeniu (24 płaszczyzny) generują najwyższy narzut obliczeniowy dla algorytmu Dijkstry (ponad 30 ms per przeliczenie), a trasy ulegają zmianie bardzo często (ok. 3-4 razy na minutę).

### 3.4. Analiza Kolejek
 
Włączenie do symulacji przepływu pakietowego pozwoliło zbadać utylizację buforów satelit.

Konsekwencje przepełnienia kolejek stają się wyraźnie widoczne na poziomie opóźnień odczuwanych przez aplikacje końcowe. Aby zbadać ten wpływ, analizujemy scenariusz D, w którym ruch pomiarowy (ping) konkurował o pasmo z intensywnym ruchem pakietowym.
<figure>
  <img src="figures/08_D_London_NY_ping_rtt_timeseries.png" alt="">
  <figcaption><b>Wykres 8.</b> Ewolucja opóźnienia RTT w czasie dla połączenia transatlantyckiego przy aktywnym obciążeniu sieciowym</figcaption>
</figure>

Wykres RTT dla scenariusza z ruchem pakietowym diametralnie różni się od gładkich wykresów z testów wyłącznie routingowych. Obserwowane tu ekstremalne i wysoce zmienne wahania (jitter) to efekt opóźnień kolejkowania (ang. queuing delay). Gdy pakiety wpadają do obciążonych buforów w satelitach, czas ich przetworzenia znacząco rośnie, degradując jakość transmisji danych wrażliwych na opóźnienia w czasie rzeczywistym.

## 4. Ograniczenia Zastosowanego Modelu

Przy interpretacji wyników należy uwzględnić następujące ograniczenia symulatora:

1. Minimalistyczny scenariusz dwusatelitarny pełni funkcję testu poprawności kodu i nie obrazują rzeczywistych własności pokrycia systemu Starlink.


2. Operujemy wyłącznie w oparciu o łącza międzysatelitarne (ISL), nie wykorzystując pośrednich, naziemnych stacji przekaźnikowych (Relay Stations), co może zawyżać obserwowane wartości RTT.


3. Skrypty działające w trybie `routing-only` dostarczają cennych metryk geometrycznych, jednak ze względu na brak generowanego ruchu pakietowego nie nadają się do analizy przepustowości, utraty pakietów ani zjawisk kolejkowania.


4. Częsta modyfikacja optymalnej ścieżki (Dijkstra) może negatywnie odbijać się na operacyjnej stabilności protokołów transportowych.


## 5. Konkluzja

Symulacja `starlink-htsim` skutecznie dowiodła skomplikowanej zależności między dynamiką orbitalną a wydajnością protokołów komputerowych. Kluczowymi wyzwaniami w sieciach typu Starlink, są zarządzanie momentami *handoveru* przeciwdziałającymi uciążliwemu zjawisku jitteru, minimalizacja narzutu obliczeniowego przy bardzo częstych aktualizacjach tabel routingu, oraz implementacja nowoczesnych algorytmów *congestion control*, zdolnych sprawiedliwie alokować współdzielone, wąskie gardła w łączach ISL.

Symulacja mogłaby zostać rozszerzona o obsługę naziemnych stacji przekaźnikowych (Relay Stations), co pozwoli na bardziej realistyczne odwzorowanie aktualnych faz wdrażania konstelacji Starlink.