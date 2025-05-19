import os
import subprocess
import pandas as pd
import numpy as np
from sklearn.cluster import KMeans
from sklearn.metrics import silhouette_score
from fastapi import HTTPException
from pathlib import Path

class NS3SimulationService:
    def __init__(self):
        self.ns3_path = os.getenv('NS3_PATH', '/home/youruser/ns-allinone-3.41/ns-3.41')
        self.scripts_path = Path(__file__).parent.parent.parent / 'scripts'
        self.scripts_path.mkdir(exist_ok=True)
        
    async def generate_gateways(self, devices: list[dict]):
        """Gera gateways a partir dos dispositivos usando K-means"""
        try:
            # Salva dispositivos em CSV
            devices_df = pd.DataFrame(devices)
            devices_file = self.scripts_path / 'devices.csv'
            devices_df.to_csv(devices_file, index=False, header=False)
            
            # Executa o clustering
            optimal_clusters = self._find_optimal_clusters(devices_df)
            gateways = self._perform_clustering(devices_df, optimal_clusters)
            
            # Salva gateways
            gateways_file = self.scripts_path / 'gateways.csv'
            gateways_df = pd.DataFrame(gateways, columns=['lat', 'lng'])
            gateways_df.to_csv(gateways_file, index=False, header=False)
            
            return gateways_df.to_dict('records')
            
        except Exception as e:
            raise HTTPException(500, f"Gateway generation failed: {str(e)}")
    
    def _find_optimal_clusters(self, df, max_clusters=10):
        """Encontra o número ótimo de clusters usando o método da silhueta"""
        n_samples = len(df)
        
        # Casos especiais para poucos dispositivos
        if n_samples <= 2:
            return 1  # Com 2 ou menos dispositivos, usa apenas 1 gateway
        
        coordinates = df[['lat', 'lng']].values
        silhouette_scores = []
        cluster_range = range(2, min(max_clusters, n_samples - 1) + 1)  # Garante n_samples - 1
        
        for k in cluster_range:
            if k >= n_samples:  # Não pode ter mais clusters que amostras
                break
                
            kmeans = KMeans(n_clusters=k, random_state=42)
            labels = kmeans.fit_predict(coordinates)
            
            # Verifica se temos pelo menos 2 amostras em cada cluster
            _, counts = np.unique(labels, return_counts=True)
            if any(counts < 2):
                continue  # Pula este k se algum cluster tiver menos de 2 pontos
                
            score = silhouette_score(coordinates, labels)
            silhouette_scores.append(score)
        
        if not silhouette_scores:  # Se nenhum k foi válido
            return min(2, n_samples)  # Retorna o mínimo entre 2 e o número de amostras
        
        optimal_k = cluster_range[np.argmax(silhouette_scores)]
        return optimal_k
    
    def _perform_clustering(self, df, n_clusters):
        """Executa o algoritmo K-means"""
        if len(df) <= 1:
            return df[['lat', 'lng']].values.tolist()  # Retorna todos os pontos como gateways
        
        n_clusters = min(n_clusters, len(df))
        if n_clusters <= 1:
            # Se apenas 1 cluster, retorna o centróide de todos os pontos
            return [[df['lat'].mean(), df['lng'].mean()]]
        
        kmeans = KMeans(n_clusters=n_clusters, random_state=42)
        kmeans.fit(df[['lat', 'lng']])
        return kmeans.cluster_centers_.tolist()
    
    async def run_simulation(self, params: dict):
        """Executa a simulação no NS3"""
        try:
            devices_file = self.scripts_path / 'devices.csv'
            gateways_file = self.scripts_path / 'gateways.csv'
            output_folder = self.scripts_path / 'output'
            output_folder.mkdir(exist_ok=True)
            
            cmd = [
                './ns3', 'run',
                'scratch/scratch-simulator.cc',
                f'--file_endevices={devices_file}',
                f'--file_gateways={gateways_file}',
                f'--out_folder={output_folder}',
            ]
            
            # Adiciona parâmetros adicionais da simulação
            if 'n_channels' in params:
                cmd.append(f'--n_channels={params["n_channels"]}')
            if 'tx_power' in params:
                cmd.append(f'--tx_power={params["tx_power"]}')
            if 'sf' in params:
                cmd.append(f'--sf={params["sf"]}')
            
            result = subprocess.run(
                cmd,
                cwd=self.ns3_path,
                capture_output=True,
                text=True
            )
            
            if result.returncode != 0:
                raise HTTPException(500, f"NS3 simulation failed: {result.stderr}")
                
            return {
                "success": True,
                "output": result.stdout,
                "parameters": params,
                "gateways_file": str(gateways_file),
                "devices_file": str(devices_file)
            }
            
        except Exception as e:
            raise HTTPException(500, f"NS3 simulation failed: {str(e)}")