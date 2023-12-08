import matplotlib.pyplot as plt
import pandas as pd

file_path = 'histo_secuencial.csv'
data = pd.read_csv(file_path)

valor = data['valor']
histo = data['histo']
eqHisto = data['eqHisto']

plt.figure(figsize=(10, 6))
plt.bar(valor, histo, alpha=0.5, label='Histograma original')
plt.bar(valor, eqHisto, alpha=0.5, label='Histograma ecualizado')
plt.xlabel('Valor')
plt.ylabel('Frecuencia')
plt.title('Histograma Original vs Histograma Ecualizado')
plt.legend()
plt.grid(True)
plt.show()