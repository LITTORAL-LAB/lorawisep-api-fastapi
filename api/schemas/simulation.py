from pydantic import BaseModel
from typing import List

class GatewayResponse(BaseModel):
    gateways: List[dict[str, float]] 

class SimulationParameters(BaseModel):
    devices: List[dict[str, float]] 
    simulationTime: int = 60
    nDevices: int = 100
    radius: float = 1000.0
    
    class Config:
        from_attributes = True