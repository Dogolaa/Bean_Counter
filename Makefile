# Nome do compilador
CC = gcc

# Opções de compilação
CFLAGS = -Wall -O2

# Nome do arquivo executável
TARGET = contafeijoes

# Nome do arquivo fonte
SRC = contafeijoes.c

# Regras do Makefile
all: $(TARGET)
	@echo "Para executar o programa, execute:"
	@echo "  ./contafeijoes image.pgm"

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -lm

# Limpeza dos arquivos gerados pela compilação
clean:
	rm -f $(TARGET)

# Instruções para o usuário
help:
	@echo "Para compilar o programa, execute:"
	@echo "  make"
	@echo "Para executar o programa, execute:"
	@echo "  ./contafeijoes image.pgm"
