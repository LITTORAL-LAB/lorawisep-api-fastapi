from fastapi import FastAPI, HTTPException
from fastapi.responses import JSONResponse
from api.routes import router
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI()

app.include_router(router, prefix="/api/v1")

app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:5173",
        "https://lorawisep.littoralab.com.br",
        "http://localhost:8888",
    ],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.exception_handler(HTTPException)
async def custom_http_exception_handler(request, exc: HTTPException):
    return JSONResponse(
        status_code=exc.status_code,
        content={"error": exc.detail},
    )

@app.get("/")
def read_root():
    return {"message": "Welcome"}