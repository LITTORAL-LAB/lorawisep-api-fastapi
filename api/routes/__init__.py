from fastapi import APIRouter

from api.routes import conta
from api.routes import usuarios
from api.routes import politicas
from api.routes import simulacao

router = APIRouter()

router.include_router(conta.router)
router.include_router(usuarios.router)
router.include_router(politicas.router)
router.include_router(simulacao.router)