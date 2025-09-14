# Etapa 1: Use uma imagem base com suporte completo
FROM ubuntu:22.04

# Definir variáveis de ambiente
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    APP_HOME=/app \
    NS3_HOME=/ns3

# Instalar dependências do sistema
RUN apt-get update && apt-get install -y \
    g++ \
    python3 \
    cmake \
    ninja-build \
    git \
    ccache \
    libgsl-dev \
    libsqlite3-dev \
    libxml2-dev \
    libpq-dev \
    python3-dev \
    python3-pip \
    poppler-utils \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Criar diretórios de trabalho
WORKDIR $NS3_HOME

# Baixar e configurar ns-3 com o módulo LoRaWAN (etapa corrigida)
RUN git clone https://gitlab.com/nsnam/ns-3-dev.git . && \
    git clone https://github.com/signetlabdei/lorawan src/lorawan && \
    cd src/lorawan && \
    NS3_VERSION=$(cat NS3-VERSION | sed 's/release //') && \
    cd ../.. && \
    git checkout -b lorawan-branch $NS3_VERSION

# Copy simulation sources before building
COPY ns3_files $NS3_HOME/scratch

# Compilar ns-3 com o módulo LoRaWAN
RUN ./ns3 configure --enable-tests --enable-examples --enable-modules lorawan && \
    ./ns3 build

# Configurar ambiente Python
WORKDIR $APP_HOME
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copiar o código da aplicação
COPY . .

# Expor a porta da aplicação
EXPOSE 8000

# Comando de inicialização
ENTRYPOINT ["sh", "-c", "alembic upgrade heads && uvicorn main:app --host 0.0.0.0 --port 8000"]