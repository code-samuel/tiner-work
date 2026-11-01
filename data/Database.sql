CREATE DATABASE IF NOT EXISTS TinerWork;

USE TinerWork;

CREATE TABLE IF NOT EXISTS Colaboradores (
    id INT NOT NULL AUTO_INCREMENT,
    nome VARCHAR(150),
    cpf CHAR(11) UNIQUE,
    data_nascimento DATE,
    data_admissao DATE,
    telefone VARCHAR(20),
    CONSTRAINT pk_colaboradores_id PRIMARY KEY(id)
);

CREATE TABLE IF NOT EXISTS Empresas (
    id INT NOT NULL AUTO_INCREMENT,
    razao_social VARCHAR(150),
    cnpj CHAR(14) UNIQUE,
    segmento VARCHAR(150),
    CONSTRAINT pk_empresas_id PRIMARY KEY (id)
);

CREATE TABLE IF NOT EXISTS Escalas (
    id INT NOT NULL AUTO_INCREMENT,
    descricao VARCHAR(50),
    carga_horaria_diaria DECIMAL(4,2),
    carga_horaria_semanal DECIMAL(5,2),
    CONSTRAINT pk_escalas_id PRIMARY KEY (id)
);

CREATE TABLE IF NOT EXISTS Contratos (
    id INT NOT NULL AUTO_INCREMENT,
    matricula VARCHAR(20) UNIQUE,
    cargo VARCHAR(100),
    data_admissao DATE,
    tempo_contrato INT,
    salario DECIMAL(10,2), 
    colaborador_id INT,
    empresa_id INT,
    escala_id INT,
    CONSTRAINT pk_contrato_id PRIMARY KEY(id),
    CONSTRAINT fk_colaborador FOREIGN KEY (colaborador_id) REFERENCES Colaboradores(id),
    CONSTRAINT fk_empresa FOREIGN KEY (empresa_id) REFERENCES Empresas(id),    
    CONSTRAINT fk_escala FOREIGN KEY (escala_id) REFERENCES Escalas(id)
);

CREATE TABLE IF NOT EXISTS Ponto (
    id INT NOT NULL AUTO_INCREMENT,
    contrato_id INT,
    escala_id INT,
    data_registro DATE,
    entrada TIME,
    saida TIME,
    CONSTRAINT pk_ponto_id PRIMARY KEY (id),
    CONSTRAINT fk_escala_id FOREIGN KEY(escala_id) REFERENCES Escalas(id),
    CONSTRAINT fk_contrato_id FOREIGN KEY(contrato_id) REFERENCES Contratos(id)
);

CREATE TABLE IF NOT EXISTS Holerites (
    id INT NOT NULL AUTO_INCREMENT,
    contrato_id INT,
    competencia DATE,
    cargo VARCHAR(100),
    salario DECIMAL(10,2),
    impostos DECIMAL(10,2),
    descontos DECIMAL(10,2),
    CONSTRAINT pk_holerite_id PRIMARY KEY(id),
    CONSTRAINT fk_contratoholerite_id FOREIGN KEY(contrato_id) REFERENCES Contratos(id)
);
