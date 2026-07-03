import matplotlib.pyplot as plt

# Słowniki do przechowywania danych dla poszczególnych połączeń (Flow IDs)
times = {}
rates = {}

# Wczytywanie pliku wygenerowanego przez Twój skrypt
nazwa_pliku = 'temp2mpxcp.txt' # Jeśli zmieniłaś nazwę na wyniki_baza_8sat.txt, popraw to tutaj

with open(nazwa_pliku, 'r') as f:
    for line in f:
        parts = line.split()
        # Sprawdzamy czy linijka ma odpowiedni format i pochodzi od XCP_SINK
        if len(parts) >= 11 and parts[2] == 'XCP_SINK':
            time = float(parts[0])
            flow_id = int(parts[4])
            rate = float(parts[10])
            
            # Inicjalizacja list dla nowych ID
            if flow_id not in times:
                times[flow_id] = []
                rates[flow_id] = []
                
            times[flow_id].append(time)
            rates[flow_id].append(rate)

# Rysowanie wykresu
plt.figure(figsize=(12, 6))

for flow_id in sorted(times.keys()):
    if len(times[flow_id]) > 0:
        plt.plot(times[flow_id], rates[flow_id], label=f'Połączenie ID: {flow_id}')

plt.xlabel('Czas symulacji (s)', fontsize=12)
plt.ylabel('Przepustowość (Rate)', fontsize=12)
plt.title('Przepustowość połączeń w czasie - Scenariusz Bazowy', fontsize=14)
plt.axvline(x=10, color='r', linestyle='--', label='Start drugiego nadajnika (10s)')
plt.legend()
plt.grid(True, linestyle=':', alpha=0.7)
plt.tight_layout()

# Wyświetl wykres i zapisz go do pliku obrazka
plt.savefig('wykres_baza.png')
plt.show()