import os
import subprocess
import pandas as pd
import numpy as np
from sklearn.cluster import KMeans
from sklearn.metrics import silhouette_score
from fastapi import HTTPException
from pathlib import Path
from loguru import logger

class NS3SimulationService:
    def __init__(self):
        self.ns3_path = os.getenv('NS3_PATH', '/home/pedro/ns-allinone-3.41/ns-3.41') # AJUSTE SEU CAMINHO AQUI se diferente
        self.scripts_path = Path(__file__).parent.parent.parent / 'scripts'
        self.scripts_path.mkdir(parents=True, exist_ok=True) # parents=True para criar diretórios pai se não existirem
        
    async def generate_gateways(self, devices: list[dict]):
        logger.info(f"DEBUG: generate_gateways received devices: {devices}")
        """Gera gateways a partir dos dispositivos usando K-means"""
        try:
            if not devices:
                # NS_LOG_WARN no C++ sugere que devemos retornar algo ou lançar erro
                # Retornar lista vazia de gateways se não houver dispositivos.
                return []

            devices_df = pd.DataFrame(devices)
            if devices_df.empty or not all(col in devices_df.columns for col in ['lat', 'lng']):
                 raise ValueError("Device data is empty or missing 'lat'/'lng' columns.")

            devices_file = self.scripts_path / 'devices.csv'
            devices_df.to_csv(devices_file, index=False, header=False)
            
            optimal_clusters = self._find_optimal_clusters(devices_df)
            gateways_coords = self._perform_clustering(devices_df, optimal_clusters)
            
            gateways_file = self.scripts_path / 'gateways.csv'
            # Se gateways_coords for uma lista de listas, crie o DataFrame corretamente
            if gateways_coords: # Somente salva se houver coordenadas de gateway
                gateways_df = pd.DataFrame(gateways_coords, columns=['lat', 'lng'])
                gateways_df.to_csv(gateways_file, index=False, header=False)
                return gateways_df.to_dict('records')
            else:
                # Se _perform_clustering retornar lista vazia (ex: 0 dispositivos),
                # cria um arquivo gateways.csv vazio e retorna lista vazia.
                with open(gateways_file, 'w') as f:
                    pass # Cria arquivo vazio
                return []
            
        except Exception as e:
            # Log da exceção para melhor depuração no servidor
            import traceback
            logger.info(f"Error in generate_gateways: {traceback.format_exc()}")
            raise HTTPException(status_code=500, detail=f"Gateway generation failed: {str(e)}")

    def _find_optimal_clusters(self, df: pd.DataFrame, max_clusters=10) -> int:
        n_samples = len(df)
        if n_samples == 0:
            return 0 # Nenhum cluster para 0 amostras
        if n_samples <= 2: # Com 1 ou 2 dispositivos, 1 gateway
            return 1
        
        coordinates = df[['lat', 'lng']].values
        silhouette_scores = []
        
        # O range para k deve ser de 2 até min(max_clusters, n_samples - 1)
        # Se n_samples for 2, n_samples - 1 é 1, então upper_bound será 1. range(2,1) é vazio.
        # Se n_samples for 3, n_samples - 1 é 2. upper_bound é min(max_clusters, 2).
        upper_bound = min(max_clusters, n_samples - 1)
        
        if upper_bound < 2: # Não há range válido para k se upper_bound < 2
            return 1 # Default para 1 cluster

        cluster_range = range(2, upper_bound + 1)
        
        tested_k_values = [] # Para armazenar os 'k' que foram realmente testados
        for k in cluster_range:
            # KMeans precisa de pelo menos k amostras para k clusters.
            # E silhouette_score precisa de pelo menos 2 labels diferentes.
            if k > n_samples: # Não deve acontecer com a lógica de upper_bound, mas é uma checagem
                continue

            kmeans = KMeans(n_clusters=k, random_state=42, n_init='auto')
            labels = kmeans.fit_predict(coordinates)
            
            unique_labels, counts = np.unique(labels, return_counts=True)
            # Silhouette score precisa de pelo menos 2 clusters e cada cluster com > 1 amostra
            # ou pelo menos 2 clusters distintos se não for possível ter >1 amostra por cluster (caso raro)
            if len(unique_labels) < 2 or (len(unique_labels) == k and any(counts < 2)): # Ajuste aqui
                continue
                
            try:
                score = silhouette_score(coordinates, labels)
                silhouette_scores.append(score)
                tested_k_values.append(k) # Adiciona o k que produziu uma pontuação
            except ValueError:
                # Acontece se todos os pontos estiverem no mesmo cluster,
                # ou se algum cluster tiver apenas 1 membro.
                continue 
        
        if not silhouette_scores:
            return 1 # Default para 1 cluster se nenhum k válido produziu uma pontuação
        
        optimal_k_index = np.argmax(silhouette_scores)
        return tested_k_values[optimal_k_index] # Retorna o k correspondente ao maior score

    def _perform_clustering(self, df: pd.DataFrame, n_clusters: int) -> list:
        n_samples = len(df)
        if n_samples == 0:
            return []
        
        # Garante que n_clusters não seja maior que o número de amostras
        # e que seja pelo menos 1 se houver amostras.
        actual_n_clusters = min(n_clusters, n_samples)
        if actual_n_clusters <= 0 and n_samples > 0:
            actual_n_clusters = 1
        elif n_samples == 0: # Se não há amostras, não há clusters
            return []

        if actual_n_clusters == 1:
            # Centróide de todos os pontos
            return [[df['lat'].mean(), df['lng'].mean()]]
        
        kmeans = KMeans(n_clusters=actual_n_clusters, random_state=42, n_init='auto')
        kmeans.fit(df[['lat', 'lng']])
        return kmeans.cluster_centers_.tolist()

    async def run_simulation(self, params: dict):
        """Executa a simulação no NS3"""
        try:
            logger.info(f"ns3_path: {self.ns3_path}")
            logger.info(f"scripts_path: {self.scripts_path}")
            devices_file = self.scripts_path / 'devices.csv'
            gateways_file = self.scripts_path / 'gateways.csv'
            output_folder = self.scripts_path / 'output'
            output_folder.mkdir(exist_ok=True)
            
            # Comando base
            cmd = [
                './ns3', 'run',
                'scratch/main.cc',
                '--'  # Este é o separador necessário para os argumentos do script
            ]      
            
            # Adiciona os parâmetros do arquivo
            cmd.extend([
                f'--file_endevices={devices_file}',
                f'--file_gateways={gateways_file}',
                f'--out_folder={output_folder}'
            ])
            
            logger.info(f"Command: {cmd}")

            result = subprocess.run(
                cmd,
                cwd=self.ns3_path,
                capture_output=True,
                text=True
            )

            # Dump full stdout/stderr to help debugging regardless of success
            logger.info(f"NS-3 return code: {result.returncode}")
            logger.info(f"NS-3 STDOUT:\n{result.stdout}")
            logger.info(f"NS-3 STDERR:\n{result.stderr}")

            if result.returncode != 0:
                raise HTTPException(
                    500,
                    f"NS3 simulation failed (code {result.returncode}): {result.stderr or result.stdout}"
                )
            
            # Processa a saída do comando para extrair os resultados
            output_lines = result.stdout.splitlines()
            found_header_line = False
            values_str = None

            for line in output_lines:
                if found_header_line:
                    values_str = line.strip()
                    break
                if line.startswith("Global MAC Packet Counts for run"):
                    found_header_line = True
            
            if values_str is None:
                error_detail = "Could not find the numeric values line after 'Global MAC Packet Counts' in NS3 output."
                logger.info(f"NS-3 Full STDOUT for error '{error_detail}':\n{result.stdout}")
                raise HTTPException(500, error_detail)

            try:
                numeric_values = [float(v) for v in values_str.split()]
                if len(numeric_values) < 3:
                    raise ValueError("Expected at least 3 numeric values.")
            except ValueError as ve:
                error_detail = f"Could not convert '{values_str}' to floats: {ve}"
                logger.info(f"NS-3 Full STDOUT for error '{error_detail}':\n{result.stdout}")
                raise HTTPException(500, error_detail)

            # Lê as posições dos gateways do arquivo CSV
            gateways_positions = []
            try:
                gateways_df = pd.read_csv(gateways_file, header=None, names=['lat', 'lng'])
                gateways_positions = gateways_df.to_dict('records')
            except Exception as e:
                logger.info(f"Warning: Could not read gateway positions: {str(e)}")

            result_dict = {
                "sent_packets": numeric_values[0],
                "received_packets": numeric_values[1],
                "received_ratio": numeric_values[2]
            }
            
            return {
                "success": True,
                "result": result_dict,
                "gateway_positions": gateways_positions,  # Adiciona as posições dos gateways
                "end_devices_positions": params,
                "gateways_file": str(gateways_file),
                "devices_file": str(devices_file),
                "raw_stdout_for_debug": result.stdout  # Opcional, para depuração
            }
            
        except Exception as e:
            raise HTTPException(500, f"NS3 simulation failed: {str(e)}")