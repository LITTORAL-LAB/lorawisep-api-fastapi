from fastapi import APIRouter, Depends

from api.core.dependencies import T_CurrentUser, T_Session, T_NS3SimulationDeps
from api.core.security import check_access, get_current_user


router = APIRouter(prefix="/simulation", tags=["Simulation"], dependencies=[Depends(get_current_user)])

@router.post("/gateways")
async def generate_gateways(
    devices: list[dict[str, float]], 
    db: T_Session, 
    deps: T_NS3SimulationDeps, 
    current_user: T_CurrentUser
):
    """Gera posições de gateways baseadas nos dispositivos"""
    check_access(db, current_user, action="Simulation:GenerateGateways")
    gateways = await deps.service.generate_gateways(devices)
    return {"data": gateways}

# [
#   {
#     "lat": 0,
#     "lng": 0
#   },
#   {
#     "lat": 10,
#     "lng": 10
#   }
# ]


@router.post("/run")
async def run_simulation(
    devices: list[dict[str, float]], 
    db: T_Session, 
    deps: T_NS3SimulationDeps, 
    current_user: T_CurrentUser
):
    """Executa a simulação completa"""
    check_access(db, current_user, action="Simulation:Run")
    
    # Primeiro gera os gateways
    await deps.service.generate_gateways(devices)
    
    # Depois roda a simulação
    result = await deps.service.run_simulation(devices)
    print("Simulation run successfully")
    
    return {
        "data": result,
        "message": "Simulation completed"
    }