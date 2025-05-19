from fastapi import APIRouter, Depends

from api.core.dependencies import T_CurrentUser, T_Session, T_NS3SimulationDeps
from api.core.response_model import SingleResponse
from api.core.security import check_access, get_current_user

from api.schemas.simulation import SimulationParameters

router = APIRouter(prefix="/simulation", tags=["Simulation"], dependencies=[Depends(get_current_user)])

@router.post("/gateways", response_model=SingleResponse)
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

@router.post("/run", response_model=SingleResponse)
async def run_simulation(
    params: SimulationParameters, 
    db: T_Session, 
    deps: T_NS3SimulationDeps, 
    current_user: T_CurrentUser
):
    """Executa a simulação completa"""
    check_access(db, current_user, action="Simulation:Run")
    
    # Primeiro gera os gateways
    await deps.service.generate_gateways([d.dict() for d in params.devices])
    
    # Depois roda a simulação
    result = await deps.service.run_simulation(params.dict())
    
    return {
        "data": result,
        "message": "Simulation completed"
    }