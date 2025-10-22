# 🪙 Loja de Joias — Estrutura de Arquivos e Índices

Este projeto implementa uma estrutura de armazenamento binário para uma loja de joias, com foco em eficiência de busca, organização ordenada e otimizações de acesso.  
Os dados são mantidos em arquivos binários (`.dat`) e indexados por arquivos auxiliares (`.idx`) com diferentes estratégias de indexação.

---

## 📂 1. Arquivos de Dados

### **1.1 produtos.dat**
Armazena informações sobre os produtos da loja, ordenados por `product_id`.

#### **Estrutura do Registro**
```c
ctypedef struct {
    long long product_id;      // 8 bytes - Identificador único do produto
    float price;               // 4 bytes - Preço em USD
    long long category_id;     // 8 bytes - ID da categoria
    char category_alias[50];   // 50 bytes - Nome/alias da categoria
    int brand_id;              // 4 bytes - ID da marca
    char gender;               // 1 byte - Gênero (M/F/U)
    char ativo;                // 1 byte - Registro ativo (1) ou deletado (0)
    char padding[4];           // 4 bytes - Padding para alinhamento
} Produto;
```

- **Tamanho total:** 80 bytes por registro  
- **Ordenação:** por `product_id`  
- **Deleção:** lógica via campo `ativo`  
- **Acesso:** sequencial, com suporte a índice parcial  
- **Formato:** binário, com `\n` separando registros  

#### **Operações Suportadas**
- Inserção ordenada  
- Deleção lógica  
- Reorganização (compactação)  
- Consulta via índice parcial  

---

### **1.2 compras.dat**
Armazena os itens de pedidos realizados pelos usuários, ordenados por `order_id`.

#### **Estrutura do Registro**
```c
ctypedef struct {
    char order_datetime[30];   // 30 bytes - Data/hora do pedido
    long long order_id;        // 8 bytes - ID único do pedido
    long long product_id;      // 8 bytes - Produto comprado
    int quantity;              // 4 bytes - Quantidade
    long long user_id;         // 8 bytes - ID do usuário
    char ativo;                // 1 byte - Registro ativo
    char padding[29];          // 29 bytes - Padding
} Compra;
```

- **Tamanho total:** 88 bytes por registro  
- **Multiplicidade:** um `order_id` pode ter vários produtos  
- **Deleção:** lógica (campo `ativo`)  
- **Formato:** binário com separador `\n`  
- **Acesso:** sequencial com índice inverso  

#### **Operações Suportadas**
- Inserção ordenada  
- Deleção de item ou pedido completo  
- Reorganização  
- Consulta via índice inverso  

---

## 🗂️ 2. Arquivos de Índices

### **2.1 produtos.idx (Índice Parcial)**
Índice **esparso**, com uma entrada a cada *BLOCO_INDICE* registros (padrão: 3).  
Permite busca **binária** seguida de varredura **sequencial limitada**.

#### **Estrutura**
```c
ctypedef struct {
    long long chave;    // product_id de referência
    long posicao;       // posição (offset) no arquivo de dados
} IndiceParcial;
```

- **Tamanho da entrada:** 17 bytes (16 + '\n')  
- **Tipo:** esparso (não denso)  
- **Granularidade:** 1 entrada / 3 registros  
- **Formato:** binário sequencial  

#### **Funcionamento**
1. Busca binária no índice  
2. Posicionamento no offset encontrado  
3. Busca sequencial em até `BLOCO_INDICE * 2` registros  

---

### **2.2 compras_inv.idx (Índice Inverso)**
Índice **denso agrupado**, que mapeia todos os registros pertencentes a um mesmo `order_id`.

#### **Estrutura**
```c
cint num_pedidos;  // Cabeçalho: total de pedidos

cstruct EntradaPedido {
    long long order_id;      // ID do pedido
    int num_itens;           // Quantidade de itens
    long posicoes[num_itens]; // Array de offsets no arquivo compras.dat
}
```

- **Tamanho variável:** 12 + (8 × num_itens) bytes  
- **Ordenação:** por `order_id`  
- **Formato:** binário agrupado  
- **Limite:** até 50 itens por pedido  

#### **Funcionamento**
1. Leitura do cabeçalho (`num_pedidos`)  
2. Busca pelo `order_id`  
3. Leitura das posições associadas  
4. Acesso direto aos itens do pedido  

---

---

## ⚙️ 4. Otimizações Implementadas

### **4.1 Deleção Lógica**
- Registros mantidos fisicamente  
- Flag `ativo` marca exclusão  
- Reorganização periódica remove registros inativos  

### **4.2 Índice Esparso**
- Reduz tamanho do índice em até **66%**  
- Mantém boa performance de busca  

### **4.3 Índice Inverso Agrupado**
- Otimiza consultas de pedidos com vários itens  
- Reduz múltiplos acessos ao arquivo principal  

### **4.4 Separador `\n`**
- Facilita depuração manual  
- Identificação visual de registros  
- Overhead mínimo (1 byte por registro)  

---

**Sistema de Arquivos Binários para Loja de Joias**  
Trabalho acadêmico desenvolvido com foco em **organização sequencial**, **índices binários** e **otimização de acesso a dados**.
