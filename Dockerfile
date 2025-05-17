# Etapa 1: Escolha uma imagem base leve com suporte ao Python
FROM python:3.13-slim

# Definir variáveis de ambiente
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    APP_HOME=/app

# Definir o diretório de trabalho
WORKDIR $APP_HOME

# Instalar dependências do sistema necessárias para psycopg2
RUN apt-get update && apt-get install -y \
    gcc \
    libpq-dev \
    python3-dev \
    poppler-utils \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Copiar apenas os arquivos necessários para instalação das dependências
COPY requirements.txt .

# Instalar as dependências do Python
RUN pip install --no-cache-dir -r requirements.txt

# Copiar o código-fonte da aplicação para o contêiner
COPY . .

# Expor a porta da aplicação
EXPOSE 8000

# Executar migrações Alembic ao iniciar o contêiner e iniciar o servidor Uvicorn
ENTRYPOINT ["sh", "-c", "alembic upgrade heads && uvicorn main:app --host 0.0.0.0 --port 8000"]