# Setup database for ACOW

CREATE DATABASE acow;

# GRANT ALL ON acow.* TO user@localhost IDENTIFIED BY "password";
# FLUSH PRIVILEGES;

USE acow;

CREATE TABLE IF NOT EXISTS transaction 
(
  id		INT AUTO_INCREMENT PRIMARY KEY,
  date		DATE,
  acc_src	INT,
  acc_dst	INT,
  value		INT,
  comment	VARCHAR(255)
);

CREATE TABLE IF NOT EXISTS account
(
  id		INT UNIQUE PRIMARY KEY,
  name		VARCHAR(32),
);

INSERT INTO account (id,name) VALUES (0,"current");
INSERT INTO account (id,name) VALUES (1,"credit card");
INSERT INTO account (id,name) VALUES (2,"savings");

INSERT INTO account (id,name) VALUES (1000,"MIN_PAYEE");
