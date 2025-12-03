#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#define MAX_STRING 100
#define BLOCO_INDICE 3

#define ORDEM_ARVORE_B 64
#define TAMANHO_HASH 10000

// ========== ESTRUTURAS ==========
typedef struct {
    long long product_id;      
    float price;              
    long long category_id;    
    char category_alias[50];   
    int brand_id;             
    char gender;            
    char ativo;             
    char padding[4];  
} Produto;

typedef struct {
    char order_datetime[30]; 
    long long order_id;        
    long long product_id;
    int quantity;            
    long long user_id;         
    char ativo;               
    char padding[29];          
} Compra;

typedef struct {
    long long chave;
    long posicao;
} IndiceParcial;

typedef struct {
    long long product_id;
    int num_compras;
    long *posicoes;
} IndiceInverso;

typedef struct NoB {
    int num_chaves;
    long long chaves[ORDEM_ARVORE_B];
    long posicoes[ORDEM_ARVORE_B];
    struct NoB *filhos[ORDEM_ARVORE_B + 1];
    int eh_folha;
} NoB;

typedef struct {
    NoB *raiz;
    int altura;
    int num_nos;
} ArvoreB;

// ========== TABELA HASH PARA COMPRAS (por user_id) ==========
typedef struct EntradaHash {
    long long order_id;    
    int num_itens;          
    long *posicoes;        
    struct EntradaHash *proximo;
} EntradaHash;

typedef struct {
    EntradaHash **tabela;
    int tamanho;
    int num_elementos;
    int colisoes;
} TabelaHash;


// ========== VARIÁVEIS GLOBAIS ==========
ArvoreB *arvore_produtos = NULL;
TabelaHash *hash_compras = NULL;

// ========== FUNÇÕES AUXILIARES ==========
int indices_carregados = 0;

int remover_arvore_b_aux(NoB *no, long long chave);

void limpar_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int arquivo_existe(const char *nome) {
    FILE *f = fopen(nome, "rb");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

void carregar_indices_automatico(const char *idx_produtos, const char *idx_compras) {
    int produtos_ok = arquivo_existe(idx_produtos);
    int compras_ok = arquivo_existe(idx_compras);
    
    if (produtos_ok && compras_ok) {
        printf("\n[INFO] Indices encontrados e carregados:\n");
        printf("  - %s\n", idx_produtos);
        printf("  - %s\n", idx_compras);
        indices_carregados = 1;
    } else {
        printf("\n[INFO] Indices não encontrados. Use a opção 2 para criá-los.\n");
        if (!produtos_ok) printf("  - Faltando: %s\n", idx_produtos);
        if (!compras_ok) printf("  - Faltando: %s\n", idx_compras);
        indices_carregados = 0;
    }
}

// ========== FUNÇÕES DE PRODUTOS ==========
void inserir_produtos(const char *arquivo) {
    FILE *csv = fopen("produtos.csv", "r");
    if (!csv) {
        printf("Erro ao abrir arquivo produtos.csv!\n");
        printf("Certifique-se de que o arquivo existe no diretório.\n");
        return;
    }
    
    FILE *fp = fopen(arquivo, "wb");
    if (!fp) {
        printf("Erro ao criar arquivo de produtos!\n");
        fclose(csv);
        return;
    }
    
    char linha[500];
    int count = 0;
    
    fgets(linha, sizeof(linha), csv);
    
    printf("\nLendo produtos do CSV...\n");
    
    while (fgets(linha, sizeof(linha), csv)) {
        Produto p;
        
        memset(&p, 0, sizeof(Produto));
        p.ativo = 1; 
        p.gender = 'u'; 
        
        char *token;
        char linha_copia[500];
        strcpy(linha_copia, linha);
        
        int campo = 0;
        char *ptr = linha_copia;
        char *inicio = ptr;
        
        while (*ptr && campo < 6) {
            if (*ptr == ',' || *ptr == '\n' || *ptr == '\r') {
                *ptr = '\0'; 
                
                switch(campo) {
                    case 0:
                        if (strlen(inicio) > 0) {
                            sscanf(inicio, "%lld", &p.product_id);
                        }
                        break;
                    case 1:
                        if (strlen(inicio) > 0) {
                            sscanf(inicio, "%f", &p.price);
                        }
                        break;
                    case 2:
                        if (strlen(inicio) > 0) {
                            sscanf(inicio, "%lld", &p.category_id);
                        }
                        break;
                    case 3:
                        if (strlen(inicio) > 0) {
                            strncpy(p.category_alias, inicio, 49);
                            p.category_alias[49] = '\0';
                        }
                        break;
                    case 4:
                        if (strlen(inicio) > 0) {
                            sscanf(inicio, "%d", &p.brand_id);
                        }
                        break;
                    case 5: 
                        if (strlen(inicio) > 0) {
                            p.gender = inicio[0];
                        }
                        break;
                }
                
                campo++;
                inicio = ptr + 1;
                
                if (*ptr == '\n' || *ptr == '\r') break;
            }
            ptr++;
        }
        
        if (p.product_id > 0) {
            fwrite(&p, sizeof(Produto), 1, fp);
            fwrite("\n", 1, 1, fp);
            count++;
            
            if (count <= 5) {
                printf("Lido: ID=%lld, Price=%.2f, Cat=%lld, Alias=%s, Brand=%d, Gender=%c\n",
                       p.product_id, p.price, p.category_id, p.category_alias, p.brand_id, p.gender);
            }
        }
    }
    
    fclose(csv);
    fclose(fp);
    
    if (count == 0) {
        printf("Nenhum dado foi lido do CSV!\n");
        return;
    }
    
    printf("Arquivo de produtos criado com %d registros.\n", count);
    printf("Tamanho de cada registro: %lu bytes\n", sizeof(Produto));
}

// Mostrar produtos
void mostrar_produtos(const char *arquivo) {
    FILE *fp = fopen(arquivo, "rb");
    if (!fp) {
        printf("Arquivo de produtos não encontrado!\n");
        return;
    }
    
    Produto p;
    char newline;
    int i = 0;
    
    printf("\n========== PRODUTOS ==========\n");
    printf("%-20s | %-10s | %-25s | %-6s\n", "Product ID", "Price", "Category", "Gender");
    printf("------------------------------------------------------------------\n");
    
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        if (p.ativo) {
            printf("%-20lld | $%-9.2f | %-25s | %c\n", 
                   p.product_id, p.price, p.category_alias, p.gender);
            i++;
        }
    }
    
    printf("Total: %d produtos\n\n", i);
    fclose(fp);
}

void criar_indice_produtos(const char *arq_dados, const char *arq_indice) {
    FILE *fp = fopen(arq_dados, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo de dados!\n");
        return;
    }
    
    FILE *fi = fopen(arq_indice, "wb");
    if (!fi) {
        printf("Erro ao criar arquivo de índice!\n");
        fclose(fp);
        return;
    }
    
    Produto p;
    char newline;
    long pos = 0;
    int count = 0;
    int indice_count = 0;
    
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        if (count % BLOCO_INDICE == 0) {
            IndiceParcial idx;
            idx.chave = p.product_id;
            idx.posicao = pos;
            fwrite(&idx, sizeof(IndiceParcial), 1, fi);
            fwrite("\n", 1, 1, fi);
            indice_count++;
        }
        
        pos = ftell(fp);
        count++;
    }
    
    fclose(fp);
    fclose(fi);
    printf("Indice parcial de produtos criado com %d entradas (1 a cada %d registros).\n", 
           indice_count, BLOCO_INDICE);
}

long pesquisa_binaria_indice(const char *arq_indice, long long chave, long *pos_inicial) {
    FILE *fi = fopen(arq_indice, "rb");
    if (!fi) {
        *pos_inicial = 0;
        return -1;
    }
    
    fseek(fi, 0, SEEK_END);
    long tamanho = ftell(fi);
    int num_indices = tamanho / (sizeof(IndiceParcial) + 1);
    
    if (num_indices == 0) {
        *pos_inicial = 0;
        fclose(fi);
        return -1;
    }
    
    int esq = 0, dir = num_indices - 1;
    int melhor_indice = -1;
    
    while (esq <= dir) {
        int meio = (esq + dir) / 2;
        fseek(fi, meio * (sizeof(IndiceParcial) + 1), SEEK_SET);
        
        IndiceParcial idx;
        char newline;
        if (fread(&idx, sizeof(IndiceParcial), 1, fi) != 1) break;
        if (fread(&newline, 1, 1, fi) != 1) break;
        
        if (idx.chave == chave) {
            *pos_inicial = idx.posicao;
            fclose(fi);
            return meio;
        } else if (idx.chave < chave) {
            melhor_indice = meio;
            esq = meio + 1;
        } else {
            dir = meio - 1;
        }
    }
    
    if (melhor_indice >= 0) {
        fseek(fi, melhor_indice * (sizeof(IndiceParcial) + 1), SEEK_SET);
        IndiceParcial idx;
        char newline;
        fread(&idx, sizeof(IndiceParcial), 1, fi);
        fread(&newline, 1, 1, fi);
        *pos_inicial = idx.posicao;
    } else {
        *pos_inicial = 0;
    }
    
    fclose(fi);
    return melhor_indice;
}

void consultar_produto(const char *arq_dados, const char *arq_indice, long long product_id) {
    long pos_inicial;
    pesquisa_binaria_indice(arq_indice, product_id, &pos_inicial);
    
    FILE *fp = fopen(arq_dados, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo de dados!\n");
        return;
    }
    
    fseek(fp, pos_inicial, SEEK_SET);
    
    Produto p;
    char newline;
    int encontrado = 0;
    int registros_lidos = 0;
    
    while (registros_lidos < BLOCO_INDICE * 2) {
        if (fread(&p, sizeof(Produto), 1, fp) != 1) break;
        if (fread(&newline, 1, 1, fp) != 1) break;
        
        registros_lidos++;
        
        if (p.product_id == product_id) {
            if (!p.ativo) {
                printf("Produto %lld foi deletado.\n", product_id);
                fclose(fp);
                return;
            }
            printf("\n=== PRODUTO ENCONTRADO ===\n");
            printf("Product ID: %lld\n", p.product_id);
            printf("Preço: $%.2f\n", p.price);
            printf("Categoria ID: %lld\n", p.category_id);
            printf("Categoria: %s\n", p.category_alias);
            printf("Marca ID: %d\n", p.brand_id);
            printf("Gênero: %c\n", p.gender);
            encontrado = 1;
            break;
        }
        
        if (p.product_id > product_id) {
            break;
        }
    }
    
    if (!encontrado) {
        printf("Produto %lld não encontrado.\n", product_id);
    }
    
    fclose(fp);
}

// ========== INSERÇÃO E DELEÇÃO DE PRODUTOS ==========

void inserir_produto_individual(const char *arq_dados, const char *arq_indice) {
    Produto novo;
    memset(&novo, 0, sizeof(Produto));
    
    printf("\n=== INSERIR NOVO PRODUTO ===\n");
    printf("Product ID: ");
    scanf("%lld", &novo.product_id);
    limpar_buffer();
    
    FILE *fp = fopen(arq_dados, "rb");
    if (fp) {
        Produto temp;
        char newline;
        int existe = 0;
        
        while (fread(&temp, sizeof(Produto), 1, fp) == 1) {
            fread(&newline, 1, 1, fp);
            if (temp.product_id == novo.product_id && temp.ativo) {
                printf("Erro: Produto com ID %lld já existe!\n", novo.product_id);
                existe = 1;
                break;
            }
        }
        fclose(fp);
        
        if (existe) return;
    }
    
    printf("Preço (USD): ");
    scanf("%f", &novo.price);
    limpar_buffer();
    
    printf("Category ID: ");
    scanf("%lld", &novo.category_id);
    limpar_buffer();
    
    printf("Category Alias: ");
    fgets(novo.category_alias, 50, stdin);
    novo.category_alias[strcspn(novo.category_alias, "\n")] = 0;
    
    printf("Brand ID: ");
    scanf("%d", &novo.brand_id);
    limpar_buffer();
    
    printf("Gênero (M/F/U): ");
    scanf("%c", &novo.gender);
    limpar_buffer();
    
    novo.ativo = 1;
    
    fp = fopen(arq_dados, "rb");
    FILE *temp_file = fopen("temp_produtos.dat", "wb");
    
    if (!fp || !temp_file) {
        printf("Erro ao abrir arquivos!\n");
        if (fp) fclose(fp);
        if (temp_file) fclose(temp_file);
        return;
    }
    
    Produto p;
    char newline;
    int inserido = 0;
    
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        if (!inserido && novo.product_id < p.product_id) {
            fwrite(&novo, sizeof(Produto), 1, temp_file);
            fwrite("\n", 1, 1, temp_file);
            inserido = 1;
        }
        
        fwrite(&p, sizeof(Produto), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    if (!inserido) {
        fwrite(&novo, sizeof(Produto), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    fclose(fp);
    fclose(temp_file);
    
    remove(arq_dados);
    rename("temp_produtos.dat", arq_dados);
    
    printf("\nProduto inserido com sucesso!\n");
}

void deletar_produto(const char *arq_dados, const char *arq_indice, long long product_id) {
    FILE *fp = fopen(arq_dados, "r+b"); 
    if (!fp) {
        printf("Erro ao abrir arquivo de produtos!\n");
        return;
    }
    
    Produto p;
    char newline;
    int encontrado = 0;
    long posicao;
    
    // Buscar o produto
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        posicao = ftell(fp) - sizeof(Produto);  
        fread(&newline, 1, 1, fp);
        
        if (p.product_id == product_id) {
            if (!p.ativo) {
                printf("Produto %lld já está deletado.\n", product_id);
                fclose(fp);
                return;
            }
            
            
            p.ativo = 0;
            
            fseek(fp, posicao, SEEK_SET);
            fwrite(&p, sizeof(Produto), 1, fp);
            fwrite("\n", 1, 1, fp);
            
            encontrado = 1;
            printf("\nProduto %lld deletado com sucesso.\n", product_id);
            break;
        }
    }
    
    fclose(fp);
    
    if (!encontrado) {
        printf("Produto %lld não encontrado.\n", product_id);
    }
}

void reorganizar_produtos(const char *arq_dados, const char *arq_indice) {
    FILE *fp = fopen(arq_dados, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo de produtos!\n");
        return;
    }
    
    FILE *temp_file = fopen("temp_produtos.dat", "wb");
    if (!temp_file) {
        printf("Erro ao criar arquivo temporário!\n");
        fclose(fp);
        return;
    }
    
    Produto p;
    char newline;
    int ativos = 0, deletados = 0;
    
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        if (p.ativo) {
            fwrite(&p, sizeof(Produto), 1, temp_file);
            fwrite("\n", 1, 1, temp_file);
            ativos++;
        } else {
            deletados++;
        }
    }
    
    fclose(fp);
    fclose(temp_file);
    
    remove(arq_dados);
    rename("temp_produtos.dat", arq_dados);
    
    printf("\n=== REORGANIZAÇÃO CONCLUÍDA ===\n");
    printf("Produtos ativos mantidos: %d\n", ativos);
    printf("Produtos deletados removidos: %d\n", deletados);
}

// ========== FUNÇÕES DE COMPRAS ==========

int parse_linha_compra(char *linha, Compra *c) {
    memset(c, 0, sizeof(Compra));
    c->ativo = 1;
    
    char *newline_pos = strchr(linha, '\n');
    if (newline_pos) *newline_pos = '\0';
    newline_pos = strchr(linha, '\r');
    if (newline_pos) *newline_pos = '\0';
    
    char *campo1 = linha;  
    char *campo2 = NULL;   
    char *campo3 = NULL;   
    char *campo4 = NULL;  
    char *campo5 = NULL; 
    
    int virgula_count = 0;
    char *ptr = linha;
    
    while (*ptr) {
        if (*ptr == ',') {
            *ptr = '\0'; 
            virgula_count++;
            
            switch(virgula_count) {
                case 1: campo2 = ptr + 1; break;
                case 2: campo3 = ptr + 1; break;
                case 3: campo4 = ptr + 1; break;
                case 4: campo5 = ptr + 1; break;
            }
        }
        ptr++;
    }
    
    if (virgula_count != 4 || !campo2 || !campo3 || !campo4 || !campo5) {
        return 0;
    }
    
    if (strlen(campo1) > 0) {
        strncpy(c->order_datetime, campo1, 29);
        c->order_datetime[29] = '\0';
    }
    
    if (strlen(campo2) > 0) {
        if (sscanf(campo2, "%lld", &c->order_id) != 1) {
            return 0;
        }
    } else {
        return 0;
    }
    
    if (strlen(campo3) > 0) {
        if (sscanf(campo3, "%lld", &c->product_id) != 1) {
            return 0;
        }
    }
    
    if (strlen(campo4) > 0) {
        if (sscanf(campo4, "%d", &c->quantity) != 1) {
            return 0;
        }
    }
    
    if (strlen(campo5) > 0) {
        if (sscanf(campo5, "%lld", &c->user_id) != 1) {
            return 0;
        }
    }
    
    return 1;
}

void inserir_compras(const char *arquivo) {
    FILE *csv = fopen("compras.csv", "r");
    if (!csv) {
        printf("Erro ao abrir arquivo compras.csv!\n");
        return;
    }
    
    FILE *fp = fopen(arquivo, "wb");
    if (!fp) {
        printf("Erro ao criar arquivo de compras!\n");
        fclose(csv);
        return;
    }
    
    char linha[500];
    int count = 0;
    int erros = 0;
    
    if (fgets(linha, sizeof(linha), csv)) {
        printf("Cabeçalho: %s", linha);
    }
    
    printf("\n[INFO] Lendo compras do CSV...\n");
    
    while (fgets(linha, sizeof(linha), csv)) {
        Compra c;
        
        if (parse_linha_compra(linha, &c)) {
            fwrite(&c, sizeof(Compra), 1, fp);
            fwrite("\n", 1, 1, fp);
            count++;
            
            if (count <= 5) {
                printf("Reg %d: Order=%lld, DateTime=%s, Prod=%lld, Qty=%d, User=%lld\n",
                       count, c.order_id, c.order_datetime, c.product_id, 
                       c.quantity, c.user_id);
            }
        } else {
            erros++;
            if (erros <= 3) {
                printf("[AVISO] Falha ao processar linha %d\n", count + erros);
            }
        }
    }
    
    fclose(csv);
    fclose(fp);
    
    if (count == 0) {
        printf("Nenhum dado foi lido do CSV!\n");
        return;
    }
    
    printf("\nArquivo de compras criado com %d registros.\n", count);
    if (erros > 0) {
        printf("Linhas com erro: %d\n", erros);
    }
    printf("Tamanho de cada registro: %lu bytes\n", sizeof(Compra));
}

void mostrar_compras(const char *arquivo) {
    FILE *fp = fopen(arquivo, "rb");
    if (!fp) {
        printf("Arquivo de compras não encontrado!\n");
        return;
    }
    
    Compra c;
    char newline;
    int i = 0;
    
    printf("\n========== COMPRAS ==========\n");
    printf("%-30s | %-20s | %-20s | Qtd | %-12s\n", "Data/Hora", "Order ID", "Product ID", "User ID");
    printf("----------------------------------------------------------------------------------------------------\n");
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        if (c.ativo) {  
            printf("%-30s | %-20lld | %-20lld | %3d | %-12lld\n", 
                   c.order_datetime, c.order_id, c.product_id, c.quantity, c.user_id);
            i++;
        }
    }
    
    printf("Total: %d compras\n\n", i);
    fclose(fp);
}

void criar_indice_inverso_compras(const char* arq_dados, const char* arq_indice) {
    FILE* fp = fopen(arq_dados, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo de compras!\n");
        return;
    }
    
    FILE* fi = fopen(arq_indice, "wb");
    if (!fi) {
        printf("Erro ao criar índice!\n");
        fclose(fp);
        return;
    }
    
    printf("[INFO] Criando índice inverso...\n");
    
    int num_pedidos = 0;
    fwrite(&num_pedidos, sizeof(int), 1, fi);
    
    Compra c;
    char newline;
    
    long long order_id_atual = -1;
    long posicoes_temp[50];  
    int count_itens = 0;
    int total_registros = 0;
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        long pos = ftell(fp) - sizeof(Compra) - 1;
        
        total_registros++;
        
        if (total_registros <= 5) {
            printf("  Lendo reg %d: Order=%lld, Pos=%ld, Ativo=%d\n", 
                   total_registros, c.order_id, pos, c.ativo);
        }
        
        if (c.ativo) {
            if (c.order_id != order_id_atual) {
                
                if (order_id_atual != -1 && count_itens > 0) {
                    fwrite(&order_id_atual, sizeof(long long), 1, fi);
                    fwrite(&count_itens, sizeof(int), 1, fi);
                    fwrite(posicoes_temp, sizeof(long), count_itens, fi);
                    num_pedidos++;
                    
                    if (num_pedidos <= 3) {
                        printf("  -> Gravando pedido %d: Order=%lld, Itens=%d\n", 
                               num_pedidos, order_id_atual, count_itens);
                    }
                }
                
                order_id_atual = c.order_id;
                count_itens = 0;
            }
            
            if (count_itens < 50) {
                posicoes_temp[count_itens++] = pos;
            } else {
                printf("[AVISO] Pedido %lld excedeu limite de 50 itens!\n", 
                       order_id_atual);
            }
        }
    }
    
    if (order_id_atual != -1 && count_itens > 0) {
        fwrite(&order_id_atual, sizeof(long long), 1, fi);
        fwrite(&count_itens, sizeof(int), 1, fi);
        fwrite(posicoes_temp, sizeof(long), count_itens, fi);
        num_pedidos++;
        printf("  -> Gravando último pedido: Order=%lld, Itens=%d\n", 
               order_id_atual, count_itens);
    }
    
    fseek(fi, 0, SEEK_SET);
    fwrite(&num_pedidos, sizeof(int), 1, fi);
    
    fclose(fp);
    fclose(fi);
    
    printf("\n[SUCESSO] Índice inverso criado:\n");
    printf("  - Total de registros lidos: %d\n", total_registros);
    printf("  - Total de pedidos únicos: %d\n", num_pedidos);
}

int busca_binaria_indice_inverso(const char *arq_indice, long long order_id, 
                                          int *num_itens_out, long **posicoes_out) {
    FILE *fi = fopen(arq_indice, "rb");
    if (!fi) {
        printf("Nao foi possível abrir o índice!\n");
        return -1;
    }
    
    int num_pedidos;
    if (fread(&num_pedidos, sizeof(int), 1, fi) != 1) {
        printf("Nao foi possível ler número de pedidos!\n");
        fclose(fi);
        return -1;
    }
    
    printf("[DEBUG] Buscando Order ID %lld em %d pedidos...\n", order_id, num_pedidos);
    
    if (num_pedidos == 0) {
        printf("Indice vazio!\n");
        fclose(fi);
        return -1;
    }
    
    long long oid;
    int nitems;
    
    for (int i = 0; i < num_pedidos; i++) {
        if (fread(&oid, sizeof(long long), 1, fi) != 1) {
            printf("Falha ao ler order_id na posição %d\n", i);
            break;
        }
        
        if (fread(&nitems, sizeof(int), 1, fi) != 1) {
            printf("Falha ao ler num_itens na posição %d\n", i);
            break;
        }
        
        if (oid == order_id) {
            
            *num_itens_out = nitems;
            *posicoes_out = (long*)malloc(nitems * sizeof(long));
            
            if (fread(*posicoes_out, sizeof(long), nitems, fi) != (size_t)nitems) {
                free(*posicoes_out);
                fclose(fi);
                return -1;
            }
            
            for (int j = 0; j < nitems; j++) {
                printf("%ld ", (*posicoes_out)[j]);
            }
            printf("\n");
            
            fclose(fi);
            return i;
        }
        
        fseek(fi, nitems * sizeof(long), SEEK_CUR);
    }
    
    printf("Order ID %lld NÃO encontrado no indice!\n", order_id);
    fclose(fi);
    return -1;
}

void consultar_compra_por_id(const char *arq_dados, const char *arq_indice, long long order_id) {
    int num_itens;
    long *posicoes;
    
    int resultado = busca_binaria_indice_inverso(arq_indice, order_id, &num_itens, &posicoes);
    
    if (resultado == -1) {
        printf("Compra %lld não encontrada.\n", order_id);
        return;
    }
    
    FILE *fp = fopen(arq_dados, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo de dados!\n");
        free(posicoes);
        return;
    }
    
    Compra itens[50];
    int itens_ativos = 0;
    char newline;
    
    printf("Lendo %d itens do pedido...\n", num_itens);
    
    for (int i = 0; i < num_itens; i++) {
        fseek(fp, posicoes[i], SEEK_SET);
        Compra c;
        
        if (fread(&c, sizeof(Compra), 1, fp) != 1) {
            printf("[ERRO] Falha ao ler item %d na posição %ld\n", i, posicoes[i]);
            continue;
        }
        fread(&newline, 1, 1, fp);
        
        if (i < 3) {  
            printf("[DEBUG] Item %d: Order=%lld, Product=%lld, Ativo=%d\n", 
                   i+1, c.order_id, c.product_id, c.ativo);
        }
        
        if (c.ativo) {
            itens[itens_ativos++] = c;
        }
    }
    
    fclose(fp);
    free(posicoes);
    
    if (itens_ativos == 0) {
        printf("Compra %lld foi deletada (todos os itens inativos).\n", order_id);
        return;
    }
    
    if (itens_ativos == 1) {
        printf("\n=== COMPRA ENCONTRADA ===\n");
        printf("Order ID: %lld\n", itens[0].order_id);
        printf("Data/Hora: %s\n", itens[0].order_datetime);
        printf("Product ID: %lld\n", itens[0].product_id);
        printf("Quantidade: %d\n", itens[0].quantity);
        printf("User ID: %lld\n", itens[0].user_id);
    } else {
        printf("\n=== COMPRA COM MÚLTIPLOS ITENS ===\n");
        printf("Order ID: %lld\n", order_id);
        printf("Data/Hora: %s\n", itens[0].order_datetime);
        printf("User ID: %lld\n", itens[0].user_id);
        printf("Total de itens ativos: %d\n\n", itens_ativos);
        
        for (int i = 0; i < itens_ativos; i++) {
            printf("--- Item %d ---\n", i + 1);
            printf("  Product ID: %lld\n", itens[i].product_id);
            printf("  Quantidade: %d\n", itens[i].quantity);
            printf("\n");
        }
    }
}

// ========== INSERÇÃO E DELEÇÃO DE COMPRAS ==========

void inserir_compra_individual(const char *arq_dados, const char *arq_indice) {
    Compra nova;
    memset(&nova, 0, sizeof(Compra));
    
    printf("\n=== INSERIR NOVA COMPRA/ITEM ===\n");
    
    printf("Order ID: ");
    scanf("%lld", &nova.order_id);
    limpar_buffer();
    
    printf("Data/Hora (formato: YYYY-MM-DD HH:MM:SS): ");
    fgets(nova.order_datetime, 30, stdin);
    nova.order_datetime[strcspn(nova.order_datetime, "\n")] = 0;
    
    printf("Product ID: ");
    scanf("%lld", &nova.product_id);
    limpar_buffer();
    
    printf("Quantidade: ");
    scanf("%d", &nova.quantity);
    limpar_buffer();
    
    printf("User ID: ");
    scanf("%lld", &nova.user_id);
    limpar_buffer();
    
    nova.ativo = 1;  
    
    FILE *fp = fopen(arq_dados, "rb");
    FILE *temp_file = fopen("temp_compras.dat", "wb");
    
    if (!fp || !temp_file) {
        printf("Erro ao abrir arquivos!\n");
        if (fp) fclose(fp);
        if (temp_file) fclose(temp_file);
        return;
    }
    
    Compra c;
    char newline;
    int inserido = 0;
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        if (!inserido && nova.order_id < c.order_id) {
            fwrite(&nova, sizeof(Compra), 1, temp_file);
            fwrite("\n", 1, 1, temp_file);
            inserido = 1;
        }
        
        fwrite(&c, sizeof(Compra), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    if (!inserido) {
        fwrite(&nova, sizeof(Compra), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    fclose(fp);
    fclose(temp_file);
    
    remove(arq_dados);
    rename("temp_compras.dat", arq_dados);
    
    printf("\nCompra/Item inserido com sucesso!\n");
}

// Deletar item específico de uma compra
void deletar_item_compra(const char *arq_dados, const char *arq_indice, long long order_id, long long product_id) {
    FILE *fp = fopen(arq_dados, "r+b");
    if (!fp) {
        printf("Erro ao abrir arquivo de compras!\n");
        return;
    }
    
    Compra c;
    char newline;
    int encontrado = 0;
    long posicao;
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        posicao = ftell(fp) - sizeof(Compra);  
        fread(&newline, 1, 1, fp);
        
        if (c.order_id == order_id && c.product_id == product_id) {
            if (!c.ativo) {
                printf("Este item ja esta deletado.\n");
                fclose(fp);
                return;
            }
            
            c.ativo = 0;
            
            fseek(fp, posicao, SEEK_SET);
            fwrite(&c, sizeof(Compra), 1, fp);
            fwrite("\n", 1, 1, fp);
            
            encontrado = 1;
            printf("\nItem (Order %lld, Product %lld) deletado com sucesso.\n", order_id, product_id);
            break;
        }
    }
    
    fclose(fp);
    
    if (!encontrado) {
        printf("Item nao encontrado (Order %lld, Product %lld).\n", order_id, product_id);
    }
}

void deletar_compra_completa(const char *arq_dados, const char *arq_indice, long long order_id) {
    FILE *fp = fopen(arq_dados, "r+b");
    if (!fp) {
        printf("Erro ao abrir arquivo de compras!\n");
        return;
    }
    
    Compra c;
    char newline;
    int itens_deletados = 0;
    long posicao;
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        posicao = ftell(fp) - sizeof(Compra);
        fread(&newline, 1, 1, fp);
        
        if (c.order_id == order_id && c.ativo) {
            c.ativo = 0;
            
            long pos_atual = ftell(fp);
            fseek(fp, posicao, SEEK_SET);
            fwrite(&c, sizeof(Compra), 1, fp);
            fwrite("\n", 1, 1, fp);
            fseek(fp, pos_atual, SEEK_SET);
            
            itens_deletados++;
        }
    }
    
    fclose(fp);
    
    if (itens_deletados > 0) {
        printf("\nCompra %lld deletada: %d item(s) marcado(s) como inativo(s).\n", 
               order_id, itens_deletados);
    } else {
        printf("Nenhum item ativo encontrado para a compra %lld.\n", order_id);
    }
}

void reorganizar_compras(const char *arq_dados, const char *arq_indice) {
    FILE *fp = fopen(arq_dados, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo de compras!\n");
        return;
    }
    
    FILE *temp_file = fopen("temp_compras.dat", "wb");
    if (!temp_file) {
        printf("Erro ao criar arquivo temporario!\n");
        fclose(fp);
        return;
    }
    
    Compra c;
    char newline;
    int ativos = 0, deletados = 0;
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        if (c.ativo) {
            fwrite(&c, sizeof(Compra), 1, temp_file);
            fwrite("\n", 1, 1, temp_file);
            ativos++;
        } else {
            deletados++;
        }
    }
    
    fclose(fp);
    fclose(temp_file);
    
    remove(arq_dados);
    rename("temp_compras.dat", arq_dados);
    
    printf("\n=== REORGANIZACAO DE COMPRAS CONCLUIDA ===\n");
    printf("Itens ativos mantidos: %d\n", ativos);
    printf("Itens deletados removidos: %d\n", deletados);
}

NoB* criar_no_b(int eh_folha) {
    NoB *no = (NoB*)malloc(sizeof(NoB));
    no->num_chaves = 0;
    no->eh_folha = eh_folha;
    
    for (int i = 0; i <= ORDEM_ARVORE_B; i++) {
        no->filhos[i] = NULL;
    }
    
    return no;
}

ArvoreB* criar_arvore_b() {
    ArvoreB *arvore = (ArvoreB*)malloc(sizeof(ArvoreB));
    arvore->raiz = criar_no_b(1);
    arvore->altura = 1;
    arvore->num_nos = 1;
    return arvore;
}

void dividir_filho_b(NoB *pai, int i, NoB *filho) {
    int meio = ORDEM_ARVORE_B / 2;
    NoB *novo = criar_no_b(filho->eh_folha);
    
   
    novo->num_chaves = filho->num_chaves - meio - 1;  
    
    for (int j = 0; j < novo->num_chaves; j++) {
        novo->chaves[j] = filho->chaves[meio + 1 + j];
        novo->posicoes[j] = filho->posicoes[meio + 1 + j];
    }
    
    if (!filho->eh_folha) {
        for (int j = 0; j <= novo->num_chaves; j++) {
            novo->filhos[j] = filho->filhos[meio + 1 + j];
        }
    }
    
    filho->num_chaves = meio;
    
    for (int j = pai->num_chaves; j > i; j--) {
        pai->filhos[j + 1] = pai->filhos[j];
    }
    pai->filhos[i + 1] = novo;
    
    for (int j = pai->num_chaves - 1; j >= i; j--) {
        pai->chaves[j + 1] = pai->chaves[j];
        pai->posicoes[j + 1] = pai->posicoes[j];
    }
    
    pai->chaves[i] = filho->chaves[meio];
    pai->posicoes[i] = filho->posicoes[meio];
    pai->num_chaves++;
}

void inserir_nao_cheio_b(NoB *no, long long chave, long posicao) {
    int i = no->num_chaves - 1;
    
    if (no->eh_folha) {
        while (i >= 0 && chave < no->chaves[i]) {
            no->chaves[i + 1] = no->chaves[i];
            no->posicoes[i + 1] = no->posicoes[i];
            i--;
        }
        no->chaves[i + 1] = chave;
        no->posicoes[i + 1] = posicao;
        no->num_chaves++;
    } else {
        while (i >= 0 && chave < no->chaves[i]) {
            i--;
        }
        i++;
        
        if (no->filhos[i]->num_chaves == ORDEM_ARVORE_B) {
            dividir_filho_b(no, i, no->filhos[i]);
            if (chave > no->chaves[i]) {
                i++;
            }
        }
        inserir_nao_cheio_b(no->filhos[i], chave, posicao);
    }
}

void inserir_arvore_b(ArvoreB *arvore, long long chave, long posicao) {
    NoB *raiz = arvore->raiz;
    
    if (raiz->num_chaves == ORDEM_ARVORE_B) {
        NoB *nova_raiz = criar_no_b(0);
        nova_raiz->filhos[0] = raiz;
        dividir_filho_b(nova_raiz, 0, raiz);
        arvore->raiz = nova_raiz;
        arvore->altura++;
        arvore->num_nos++;
        inserir_nao_cheio_b(nova_raiz, chave, posicao);
    } else {
        inserir_nao_cheio_b(raiz, chave, posicao);
    }
}

long buscar_arvore_b(NoB *no, long long chave) {
    if (no == NULL) return -1;
    
    int i = 0;
    while (i < no->num_chaves && chave > no->chaves[i]) {
        i++;
    }
    
    if (i < no->num_chaves && chave == no->chaves[i]) {
        return no->posicoes[i];  
    }
    
    if (no->eh_folha) {
        return -1;  
    }
    
    return buscar_arvore_b(no->filhos[i], chave);
}

void liberar_arvore_b(NoB *no) {
    if (no == NULL) return;
    
    if (!no->eh_folha) {
        for (int i = 0; i <= no->num_chaves; i++) {
            liberar_arvore_b(no->filhos[i]);
        }
    }
    free(no);
}

// ========== IMPLEMENTAÇÃO TABELA HASH ==========

TabelaHash* criar_tabela_hash(int tamanho) {
    TabelaHash *tabela = (TabelaHash*)malloc(sizeof(TabelaHash));
    tabela->tamanho = tamanho;
    tabela->num_elementos = 0;
    tabela->colisoes = 0;
    tabela->tabela = (EntradaHash**)calloc(tamanho, sizeof(EntradaHash*));
    return tabela;
}

unsigned long hash_function(long long order_id, int tamanho) {
    return (unsigned long)order_id % tamanho;
}

void inserir_hash(TabelaHash *tabela, long long order_id, long posicao) {
    unsigned long indice = hash_function(order_id, tabela->tamanho);
    
    EntradaHash *atual = tabela->tabela[indice];
    while (atual != NULL) {
        if (atual->order_id == order_id) {
            atual->num_itens++;
            atual->posicoes = (long*)realloc(atual->posicoes, atual->num_itens * sizeof(long));
            atual->posicoes[atual->num_itens - 1] = posicao;
            return;
        }
        atual = atual->proximo;
    }
    
    EntradaHash *nova = (EntradaHash*)malloc(sizeof(EntradaHash));
    nova->order_id = order_id;
    nova->num_itens = 1;
    nova->posicoes = (long*)malloc(sizeof(long));
    nova->posicoes[0] = posicao;
    nova->proximo = NULL;
    
    if (tabela->tabela[indice] == NULL) {
        tabela->tabela[indice] = nova;
    } else {
        tabela->colisoes++;
        nova->proximo = tabela->tabela[indice];
        tabela->tabela[indice] = nova;
    }
    
    tabela->num_elementos++;
}

EntradaHash* buscar_hash(TabelaHash *tabela, long long order_id) {
    unsigned long indice = hash_function(order_id, tabela->tamanho);
    EntradaHash *atual = tabela->tabela[indice];
    
    while (atual != NULL) {
        if (atual->order_id == order_id) {
            return atual;
        }
        atual = atual->proximo;
    }
    
    return NULL;
}

void liberar_tabela_hash(TabelaHash *tabela) {
    for (int i = 0; i < tabela->tamanho; i++) {
        EntradaHash *atual = tabela->tabela[i];
        while (atual != NULL) {
            EntradaHash *temp = atual;
            atual = atual->proximo;
            free(temp->posicoes);  
            free(temp);
        }
    }
    free(tabela->tabela);
    free(tabela);
}

int remover_hash(TabelaHash *tabela, long long order_id, long posicao) {
    unsigned long indice = hash_function(order_id, tabela->tamanho);
    EntradaHash *atual = tabela->tabela[indice];
    
    while (atual != NULL) {
        if (atual->order_id == order_id) {
            for (int i = 0; i < atual->num_itens; i++) {
                if (atual->posicoes[i] == posicao) {
                    for (int j = i; j < atual->num_itens - 1; j++) {
                        atual->posicoes[j] = atual->posicoes[j + 1];
                    }
                    atual->num_itens--;
                    
                    if (atual->num_itens > 0) {
                        atual->posicoes = (long*)realloc(atual->posicoes, atual->num_itens * sizeof(long));
                    }
                    
                    return 1; 
                }
            }
            return 0;
        }
        atual = atual->proximo;
    }
    
    return 0; 
}

int remover_todas_hash(TabelaHash *tabela, long long order_id) {
    unsigned long indice = hash_function(order_id, tabela->tamanho);
    EntradaHash *atual = tabela->tabela[indice];
    EntradaHash *anterior = NULL;
    
    while (atual != NULL) {
        if (atual->order_id == order_id) {
            int itens_removidos = atual->num_itens;
            
            if (anterior == NULL) {
                tabela->tabela[indice] = atual->proximo;
            } else {
                anterior->proximo = atual->proximo;
            }
            
            free(atual->posicoes);
            free(atual);
            tabela->num_elementos--;
            
            return itens_removidos;
        }
        anterior = atual;
        atual = atual->proximo;
    }
    
    return 0;
}
// ========== CRIAÇÃO DOS ÍNDICES EM MEMÓRIA ==========

void criar_indice_arvore_produtos(const char *arq_dados) {
    printf("\n[CRIANDO ÁRVORE B PARA PRODUTOS]\n");
    printf("Ordem da árvore: %d\n", ORDEM_ARVORE_B);
    
    double inicio = get_time_ms();
    
    FILE *fp = fopen(arq_dados, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo de produtos!\n");
        return;
    }
    
    if (arvore_produtos != NULL) {
        liberar_arvore_b(arvore_produtos->raiz);
        free(arvore_produtos);
    }
    
    arvore_produtos = criar_arvore_b();
    
    Produto p;
    char newline;
    long pos = 0;
    int count = 0;
    
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        if (p.ativo) {
            inserir_arvore_b(arvore_produtos, p.product_id, pos);
            count++;
        }
        
        pos = ftell(fp);
    }
    
    fclose(fp);
    
    double fim = get_time_ms();
    double tempo = fim - inicio;
    
    printf("? Árvore B criada com sucesso!\n");
    printf("  - Registros indexados: %d\n", count);
    printf("  - Altura da árvore: %d\n", arvore_produtos->altura);
    printf("  - Número de nós: %d\n", arvore_produtos->num_nos);
    printf("  - TEMPO DE CRIAÇÃO: %.2f ms\n", tempo);
}

void criar_indice_hash_compras(const char *arq_dados) {
    printf("\n[CRIANDO TABELA HASH PARA COMPRAS (order_id)]\n");
    printf("Tamanho da tabela: %d\n", TAMANHO_HASH);
    printf("Resolução de colisões: Encadeamento\n\n");
    
    double inicio = get_time_ms();
    
    FILE *fp = fopen(arq_dados, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo de compras!\n");
        return;
    }
    
    if (hash_compras != NULL) {
        liberar_tabela_hash(hash_compras);
    }
    
    hash_compras = criar_tabela_hash(TAMANHO_HASH);
    
    Compra c;
    char newline;
    long pos = 0;
    int count = 0;
    int pedidos_unicos = 0;
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        if (c.ativo) {
            inserir_hash(hash_compras, c.order_id, pos);
            count++;
        }
        
        pos = ftell(fp);
    }
    
    fclose(fp);
    
    // Contar pedidos únicos
    for (int i = 0; i < hash_compras->tamanho; i++) {
        EntradaHash *atual = hash_compras->tabela[i];
        while (atual != NULL) {
            pedidos_unicos++;
            atual = atual->proximo;
        }
    }
    
    double fim = get_time_ms();
    double tempo = fim - inicio;
    
    printf("? Tabela Hash criada com sucesso!\n");
    printf("  - Itens indexados: %d\n", count);
    printf("  - Pedidos únicos: %d\n", pedidos_unicos);
    printf("  - Colisões: %d\n", hash_compras->colisoes);
    printf("  - Fator de carga: %.2f%%\n", (pedidos_unicos * 100.0) / TAMANHO_HASH);
    printf("  - TEMPO DE CRIAÇÃO: %.2f ms\n", tempo);
}
// ========== CONSULTAS COMPARATIVAS ==========

void consultar_produto_arquivo(const char *arq_dados, const char *arq_indice, long long product_id) {
    printf("\n[BUSCA POR ÍNDICE EM ARQUIVO]\n");
    double inicio = get_time_ms();
    
    // Implementação simplificada de busca binária no índice de arquivo
    FILE *fi = fopen(arq_indice, "rb");
    FILE *fp = fopen(arq_dados, "rb");
    
    if (!fi || !fp) {
        printf("Erro ao abrir arquivos!\n");
        if (fi) fclose(fi);
        if (fp) fclose(fp);
        return;
    }
    
    // Busca no índice e depois no arquivo
    long pos_encontrada = 0;
    int encontrado = 0;
    
    // Simulação de busca (simplificada)
    fseek(fp, 0, SEEK_SET);
    Produto p;
    char newline;
    
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        if (p.product_id == product_id && p.ativo) {
            encontrado = 1;
            break;
        }
    }
    
    fclose(fi);
    fclose(fp);
    
    double fim = get_time_ms();
    
    if (encontrado) {
        printf("? Produto encontrado: %lld\n", product_id);
        printf("  Preço: $%.2f\n", p.price);
    } else {
        printf("? Produto não encontrado\n");
    }
    printf("  TEMPO: %.4f ms\n", fim - inicio);
}

void consultar_produto_memoria(const char *arq_dados, long long product_id) {
    printf("\n[BUSCA POR ARVORE B EM MEMORIA]\n");
    
    if (arvore_produtos == NULL) {
        printf("Erro: Arvore B não foi criada!\n");
        return;
    }
    
    double inicio = get_time_ms();
    
    long posicao = buscar_arvore_b(arvore_produtos->raiz, product_id);
    
    if (posicao == -1) {
        double fim = get_time_ms();
        printf("? Produto nao encontrado\n");
        printf("  TEMPO: %.4f ms\n", fim - inicio);
        return;
    }
    
    FILE *fp = fopen(arq_dados, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo!\n");
        return;
    }
    
    fseek(fp, posicao, SEEK_SET);
    Produto p;
    char newline;
    fread(&p, sizeof(Produto), 1, fp);
    fread(&newline, 1, 1, fp);
    fclose(fp);
    
    double fim = get_time_ms();
    
    printf("? Produto encontrado: %lld\n", p.product_id);
    printf("  Preco: $%.2f\n", p.price);
    printf("  Categoria: %s\n", p.category_alias);
    printf("  TEMPO: %.4f ms\n", fim - inicio);
}

void consultar_compras_usuario_arquivo(const char *arq_dados, long long user_id) {
    printf("\n[BUSCA POR USER_ID - ARQUIVO]\n");
    double inicio = get_time_ms();
    
    FILE *fp = fopen(arq_dados, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo!\n");
        return;
    }
    
    Compra c;
    char newline;
    int count = 0;
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        if (c.ativo && c.user_id == user_id) {
            count++;
        }
    }
    
    fclose(fp);
    
    double fim = get_time_ms();
    
    printf("? Encontradas %d compras do usuário %lld\n", count, user_id);
    printf("  TEMPO: %.4f ms\n", fim - inicio);
}

void consultar_compra_hash_memoria(const char *arq_dados, long long order_id) {
    printf("\n[BUSCA POR ORDER_ID - HASH EM MEMORIA]\n");
    
    if (hash_compras == NULL) {
        printf("Erro: Tabela Hash não foi criada!\n");
        return;
    }
    
    double inicio = get_time_ms();
    
    EntradaHash *entrada = buscar_hash(hash_compras, order_id);
    
    if (entrada == NULL) {
        double fim = get_time_ms();
        printf("? Compra %lld nao encontrada\n", order_id);
        printf("  TEMPO: %.4f ms\n", fim - inicio);
        return;
    }
    
    FILE *fp = fopen(arq_dados, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo!\n");
        return;
    }
    
    Compra itens[50];
    int itens_ativos = 0;
    char newline;
    
    // Ler todos os itens do pedido
    for (int i = 0; i < entrada->num_itens; i++) {
        fseek(fp, entrada->posicoes[i], SEEK_SET);
        Compra c;
        
        if (fread(&c, sizeof(Compra), 1, fp) != 1) {
            continue;
        }
        fread(&newline, 1, 1, fp);
        
        if (c.ativo) {
            itens[itens_ativos++] = c;
        }
    }
    
    fclose(fp);
    
    double fim = get_time_ms();
    
    if (itens_ativos == 0) {
        printf("? Compra %lld foi deletada (todos os itens inativos).\n", order_id);
        printf("  TEMPO: %.4f ms\n", fim - inicio);
        return;
    }
    
    printf("? Compra encontrada: %lld\n", order_id);
    printf("  Data/Hora: %s\n", itens[0].order_datetime);
    printf("  User ID: %lld\n", itens[0].user_id);
    printf("  Total de itens ativos: %d\n", itens_ativos);
    
    for (int i = 0; i < itens_ativos; i++) {
        printf("  Item %d: Product %lld (Qty: %d)\n", 
               i + 1, itens[i].product_id, itens[i].quantity);
    }
    
    printf("  TEMPO: %.4f ms\n", fim - inicio);
}
// ========== REMOÇÃO COM ATUALIZAÇÃO DE ÍNDICES ==========



// Encontra a chave predecessora (maior chave da subárvore esquerda)
long long encontrar_predecessor(NoB *no) {
    while (!no->eh_folha) {
        no = no->filhos[no->num_chaves];
    }
    return no->chaves[no->num_chaves - 1];
}

// Encontra a chave sucessora (menor chave da subárvore direita)
long long encontrar_sucessor(NoB *no) {
    while (!no->eh_folha) {
        no = no->filhos[0];
    }
    return no->chaves[0];
}

// Remove de um nó folha
void remover_de_folha(NoB *no, int idx) {
    for (int i = idx + 1; i < no->num_chaves; i++) {
        no->chaves[i - 1] = no->chaves[i];
        no->posicoes[i - 1] = no->posicoes[i];
    }
    no->num_chaves--;
}

// Empresta uma chave do irmão anterior
void emprestar_do_anterior(NoB *pai, int idx) {
    NoB *filho = pai->filhos[idx];
    NoB *irmao = pai->filhos[idx - 1];
    
    // Mover todas as chaves do filho uma posição à direita
    for (int i = filho->num_chaves - 1; i >= 0; i--) {
        filho->chaves[i + 1] = filho->chaves[i];
        filho->posicoes[i + 1] = filho->posicoes[i];
    }
    
    // Mover ponteiros de filhos se não for folha
    if (!filho->eh_folha) {
        for (int i = filho->num_chaves; i >= 0; i--) {
            filho->filhos[i + 1] = filho->filhos[i];
        }
    }
    
    // Mover chave do pai para o filho
    filho->chaves[0] = pai->chaves[idx - 1];
    filho->posicoes[0] = pai->posicoes[idx - 1];
    
    // Mover última chave do irmão para o pai
    pai->chaves[idx - 1] = irmao->chaves[irmao->num_chaves - 1];
    pai->posicoes[idx - 1] = irmao->posicoes[irmao->num_chaves - 1];
    
    // Mover último filho do irmão se não for folha
    if (!filho->eh_folha) {
        filho->filhos[0] = irmao->filhos[irmao->num_chaves];
    }
    
    filho->num_chaves++;
    irmao->num_chaves--;
}

// Empresta uma chave do irmão posterior
void emprestar_do_proximo(NoB *pai, int idx) {
    NoB *filho = pai->filhos[idx];
    NoB *irmao = pai->filhos[idx + 1];
    
    // Mover chave do pai para o filho
    filho->chaves[filho->num_chaves] = pai->chaves[idx];
    filho->posicoes[filho->num_chaves] = pai->posicoes[idx];
    
    // Mover primeiro filho do irmão se não for folha
    if (!filho->eh_folha) {
        filho->filhos[filho->num_chaves + 1] = irmao->filhos[0];
    }
    
    // Mover primeira chave do irmão para o pai
    pai->chaves[idx] = irmao->chaves[0];
    pai->posicoes[idx] = irmao->posicoes[0];
    
    // Mover todas as chaves do irmão uma posição à esquerda
    for (int i = 1; i < irmao->num_chaves; i++) {
        irmao->chaves[i - 1] = irmao->chaves[i];
        irmao->posicoes[i - 1] = irmao->posicoes[i];
    }
    
    // Mover ponteiros de filhos se não for folha
    if (!irmao->eh_folha) {
        for (int i = 1; i <= irmao->num_chaves; i++) {
            irmao->filhos[i - 1] = irmao->filhos[i];
        }
    }
    
    filho->num_chaves++;
    irmao->num_chaves--;
}

// Mescla um filho com seu irmão
void mesclar(NoB *pai, int idx) {
    int meio = ORDEM_ARVORE_B / 2;
    NoB *filho = pai->filhos[idx];
    NoB *irmao = pai->filhos[idx + 1];
    
    // Puxar chave do pai e mesclar com irmão direito
    filho->chaves[meio - 1] = pai->chaves[idx];
    filho->posicoes[meio - 1] = pai->posicoes[idx];
    
    // Copiar chaves do irmão para o filho
    for (int i = 0; i < irmao->num_chaves; i++) {
        filho->chaves[i + meio] = irmao->chaves[i];
        filho->posicoes[i + meio] = irmao->posicoes[i];
    }
    
    // Copiar ponteiros de filhos se não for folha
    if (!filho->eh_folha) {
        for (int i = 0; i <= irmao->num_chaves; i++) {
            filho->filhos[i + meio] = irmao->filhos[i];
        }
    }
    
    // Mover chaves do pai uma posição à esquerda
    for (int i = idx + 1; i < pai->num_chaves; i++) {
        pai->chaves[i - 1] = pai->chaves[i];
        pai->posicoes[i - 1] = pai->posicoes[i];
    }
    
    // Mover ponteiros de filhos do pai
    for (int i = idx + 2; i <= pai->num_chaves; i++) {
        pai->filhos[i - 1] = pai->filhos[i];
    }
    
    filho->num_chaves += irmao->num_chaves;
    pai->num_chaves--;
    
    free(irmao);
}

// Preenche o filho que tem menos que o mínimo de chaves
void preencher(NoB *pai, int idx) {
    int meio = ORDEM_ARVORE_B / 2;
    
    // Se o irmão anterior tem mais que o mínimo, empresta dele
    if (idx != 0 && pai->filhos[idx - 1]->num_chaves >= meio) {
        emprestar_do_anterior(pai, idx);
    }
    // Se o irmão posterior tem mais que o mínimo, empresta dele
    else if (idx != pai->num_chaves && pai->filhos[idx + 1]->num_chaves >= meio) {
        emprestar_do_proximo(pai, idx);
    }
    // Mescla com irmão
    else {
        if (idx != pai->num_chaves) {
            mesclar(pai, idx);
        } else {
            mesclar(pai, idx - 1);
        }
    }
}

// Remove de um nó interno
void remover_de_interno(NoB *no, int idx) {
    long long chave = no->chaves[idx];
    int meio = ORDEM_ARVORE_B / 2;
    
    if (no->filhos[idx]->num_chaves >= meio) {
        // Pegar predecessor
        long long pred = encontrar_predecessor(no->filhos[idx]);
        no->chaves[idx] = pred;
        remover_arvore_b_aux(no->filhos[idx], pred);
    }
    else if (no->filhos[idx + 1]->num_chaves >= meio) {
        // Pegar sucessor
        long long succ = encontrar_sucessor(no->filhos[idx + 1]);
        no->chaves[idx] = succ;
        remover_arvore_b_aux(no->filhos[idx + 1], succ);
    }
    else {
        // Mesclar filho com irmão
        mesclar(no, idx);
        remover_arvore_b_aux(no->filhos[idx], chave);
    }
}

// Função auxiliar recursiva de remoção
int remover_arvore_b_aux(NoB *no, long long chave) {
    if (no == NULL) return 0;
    
    int idx = 0;
    while (idx < no->num_chaves && chave > no->chaves[idx]) {
        idx++;
    }
    
    int meio = ORDEM_ARVORE_B / 2;
    
    if (idx < no->num_chaves && chave == no->chaves[idx]) {
        // Chave encontrada
        if (no->eh_folha) {
            remover_de_folha(no, idx);
        } else {
            remover_de_interno(no, idx);
        }
        return 1;
    }
    else if (no->eh_folha) {
        // Chave não encontrada
        return 0;
    }
    else {
        // Chave está em subárvore
        int flag = (idx == no->num_chaves) ? 1 : 0;
        
        if (no->filhos[idx]->num_chaves < meio) {
            preencher(no, idx);
        }
        
        if (flag && idx > no->num_chaves) {
            return remover_arvore_b_aux(no->filhos[idx - 1], chave);
        } else {
            return remover_arvore_b_aux(no->filhos[idx], chave);
        }
    }
}

// Função principal de remoção (SUBSTITUIR a antiga remover_arvore_b)
int remover_arvore_b(NoB *no, long long chave) {
    if (no == NULL) return 0;
    
    int resultado = remover_arvore_b_aux(no, chave);
    
    return resultado;
}


void deletar_produto_com_indice(const char *arq_dados, long long product_id) {
    printf("\n[DELETANDO PRODUTO %lld]\n", product_id);
    
    double inicio_total = get_time_ms();
    
    
    double inicio_arquivo = get_time_ms();
    
    FILE *fp = fopen(arq_dados, "r+b");
    if (!fp) {
        printf("Erro ao abrir arquivo de produtos!\n");
        return;
    }
    
    Produto p;
    char newline;
    int encontrado = 0;
    long posicao_registro = 0;
    
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        posicao_registro = ftell(fp) - sizeof(Produto);
        fread(&newline, 1, 1, fp);
        
        if (p.product_id == product_id) {
            if (!p.ativo) {
                printf("âœ— Produto jÃ¡ estÃ¡ deletado.\n");
                fclose(fp);
                return;
            }
            
            p.ativo = 0;
            fseek(fp, posicao_registro, SEEK_SET);
            fwrite(&p, sizeof(Produto), 1, fp);
            encontrado = 1;
            break;
        }
    }
    
    fclose(fp);
    double fim_arquivo = get_time_ms();
    
    if (!encontrado) {
        printf("âœ— Produto %lld nÃ£o encontrado.\n", product_id);
        return;
    }
    
    printf("âœ“ Produto deletado do arquivo\n");
    printf("  Tempo (arquivo): %.4f ms\n", fim_arquivo - inicio_arquivo);
    
    
    if (arvore_produtos != NULL) {
        double inicio_arvore = get_time_ms();
        
        int removido = remover_arvore_b(arvore_produtos->raiz, product_id);
        
        
        if (arvore_produtos->raiz->num_chaves == 0) {
            NoB *temp = arvore_produtos->raiz;
            if (!arvore_produtos->raiz->eh_folha) {
                arvore_produtos->raiz = arvore_produtos->raiz->filhos[0];
                arvore_produtos->altura--;
            }
            free(temp);
        }
        
        double fim_arvore = get_time_ms();
        
        if (removido) {
            printf("âœ“ Removido e reorganizado na Ãrvore B\n");
            printf("  Tempo (Ã¡rvore B): %.4f ms\n", fim_arvore - inicio_arvore);
        } else {
            printf("âœ— NÃ£o encontrado na Ãrvore B\n");
        }
    } else {
        printf("âœ— Ãrvore B nÃ£o carregada em memÃ³ria\n");
    }
    
    double fim_total = get_time_ms();
    printf("\n  TEMPO TOTAL: %.4f ms\n", fim_total - inicio_total);
}


void deletar_compra_com_indice(const char *arq_dados, long long order_id, long long product_id) {
    printf("\n[DELETANDO ITEM - Order: %lld, Product: %lld]\n", order_id, product_id);
    
    double inicio_total = get_time_ms();
    
    double inicio_arquivo = get_time_ms();
    
    FILE *fp = fopen(arq_dados, "r+b");
    if (!fp) {
        printf("Erro ao abrir arquivo de compras!\n");
        return;
    }
    
    Compra c;
    char newline;
    int encontrado = 0;
    long posicao_registro = 0;
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        posicao_registro = ftell(fp) - sizeof(Compra);
        fread(&newline, 1, 1, fp);
        
        if (c.order_id == order_id && c.product_id == product_id) {
            if (!c.ativo) {
                printf("? Item já está deletado.\n");
                fclose(fp);
                return;
            }
            
            c.ativo = 0;
            fseek(fp, posicao_registro, SEEK_SET);
            fwrite(&c, sizeof(Compra), 1, fp);
            encontrado = 1;
            break;
        }
    }
    
    fclose(fp);
    double fim_arquivo = get_time_ms();
    
    if (!encontrado) {
        printf("? Item não encontrado.\n");
        return;
    }
    
    printf("? Item deletado do arquivo\n");
    printf("  Tempo (arquivo): %.4f ms\n", fim_arquivo - inicio_arquivo);
    
    if (hash_compras != NULL) {
        double inicio_hash = get_time_ms();
        
        int removido = remover_hash(hash_compras, order_id, posicao_registro);
        
        double fim_hash = get_time_ms();
        
        if (removido) {
            printf("? Removido da Tabela Hash em memória\n");
            printf("  Tempo (hash): %.4f ms\n", fim_hash - inicio_hash);
        } else {
            printf("? Não encontrado na Hash (pode já ter sido removido)\n");
        }
    } else {
        printf("? Tabela Hash não carregada em memória\n");
    }
    
    double fim_total = get_time_ms();
    printf("\n  TEMPO TOTAL: %.4f ms\n", fim_total - inicio_total);
}

void deletar_compra_completa_com_indice(const char *arq_dados, long long order_id) {
    printf("\n[DELETANDO COMPRA COMPLETA - Order: %lld]\n", order_id);
    
    double inicio_total = get_time_ms();
    
    FILE *fp = fopen(arq_dados, "r+b");
    if (!fp) {
        printf("Erro ao abrir arquivo de compras!\n");
        return;
    }
    
    Compra c;
    char newline;
    int itens_deletados = 0;
    long posicao;
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        posicao = ftell(fp) - sizeof(Compra);
        fread(&newline, 1, 1, fp);
        
        if (c.order_id == order_id && c.ativo) {
            c.ativo = 0;
            long pos_atual = ftell(fp);
            fseek(fp, posicao, SEEK_SET);
            fwrite(&c, sizeof(Compra), 1, fp);
            fseek(fp, pos_atual, SEEK_SET);
            
            itens_deletados++;
        }
    }
    
    fclose(fp);
    double fim_arquivo = get_time_ms();
    
    if (itens_deletados == 0) {
        printf("? Nenhum item ativo encontrado para Order %lld.\n", order_id);
        return;
    }
    
    printf("? %d item(s) deletado(s) do arquivo\n", itens_deletados);
    printf("  Tempo (arquivo): %.4f ms\n", fim_arquivo - inicio_total);
    
    // Atualizar Tabela Hash - remover entrada completa
    if (hash_compras != NULL) {
        double inicio_hash = get_time_ms();
        int removidos_hash = remover_todas_hash(hash_compras, order_id);
        
        double fim_hash = get_time_ms();
        printf("? Entrada completa removida da Tabela Hash (%d itens)\n", removidos_hash);
        printf("  Tempo (hash): %.4f ms\n", fim_hash - inicio_hash);
    }
    
    double fim_total = get_time_ms();
    printf("\n  TEMPO TOTAL: %.4f ms\n", fim_total - inicio_total);
}

void teste_remocao_comparativo(const char *arq_produtos, const char *arq_compras) {
    printf("\n========================================\n");
    printf("   TESTE COMPARATIVO DE REMOÇÃO\n");
    printf("========================================\n");
    
    printf("\n--- TESTE: REMOÇÃO DE PRODUTO ---\n");
    
    long long produto_teste = 1313590958650032934;
    
    printf("\nMétodo Arquivo:\n");
    double inicio = get_time_ms();
    deletar_produto(arq_produtos, "produtos.idx", produto_teste);
    printf("  Tempo para deletar: %.4f ms\n", inicio);
    criar_indice_produtos(arq_produtos, "produtos.idx");
    double tempo_arquivo_produto = get_time_ms() - inicio;
    printf("  Tempo para recriar indice: %.4f ms\n", tempo_arquivo_produto);
    
    criar_indice_arvore_produtos(arq_produtos);
    
    long long produto_teste2 = 1313618709926904543;
    printf("\nMétodo Memória:\n");
    inicio = get_time_ms();
    deletar_produto_com_indice(arq_produtos, produto_teste2);
    printf("  Tempo para deletar: %.4f ms\n", tempo_arquivo_produto);
    
    // ========== TESTE DE COMPRAS ==========
    printf("\n--- TESTE: REMOÇÃO DE ITEM DE COMPRA ---\n");
    
    // Buscar dois order_ids diferentes para testar
    FILE *fp = fopen(arq_compras, "rb");
    Compra c;
    char newline;
    long long order_teste1= 2650244791907909836;
    long long order_teste2= 2650254276403659200;

    
    // Método 1: Arquivo (deleção + recriação de índice)
    printf("\nMétodo Arquivo:\n");
    printf("  Deletando: Order=%lld, Product=%lld\n", order_teste1);
    inicio = get_time_ms();
    deletar_compra_completa(arq_compras, "compras_inv.idx", order_teste1);
    criar_indice_inverso_compras(arq_compras, "compras_inv.idx");
    double tempo_arquivo_compra = get_time_ms() - inicio;
    printf("  Tempo: %.4f ms\n", tempo_arquivo_compra);
    
    // Recriar hash para próximo teste
    criar_indice_hash_compras(arq_compras);
    
    // Método 2: Memória (atualização incremental)
    printf("\nMétodo Memória:\n");
    printf("  Deletando: Order=%lld, Product=%lld\n", order_teste2);
    inicio = get_time_ms();
    deletar_compra_completa_com_indice(arq_compras, order_teste2);
    double tempo_memoria_compra = get_time_ms() - inicio;
    printf("  Tempo: %.4f ms\n", tempo_memoria_compra);
}

// ========== TESTES COMPARATIVOS ==========

void executar_testes_comparativos(const char *arq_produtos, const char *idx_produtos, const char *arq_compras, const char *idx_compras) {
    printf("\n");
    printf("========================================\n");
    printf("   TESTES COMPARATIVOS DE DESEMPENHO\n");
    printf("========================================\n");
    
    // IDs para teste
    long long produtos_teste[] = {1515489, 1313590958650032934, 1313618709926904543, 1515966223619268499, 1956663836006154650};
    long long orders_teste[] = {513858353, 2650244791907909836, 2650254276403659200, 2650274670577714012, 2368251069240181235};
    
    printf("\n--- TESTE 1: BUSCA DE PRODUTOS ---\n");
    for (int i = 0; i < 5; i++) {
        printf("\n>> Busca #%d - Product ID: %lld\n", i+1, produtos_teste[i]);
        printf("\n[BUSCA POR INDICE PARCIAL EM ARQUIVO]\n");
        double inicio = get_time_ms();
        consultar_produto(arq_produtos, idx_produtos, produtos_teste[i]);
        double fim = get_time_ms();
        printf("  TEMPO: %.4f ms\n", fim - inicio);
        consultar_produto_memoria(arq_produtos, produtos_teste[i]);
    }
    
    printf("\n--- TESTE 2: BUSCA DE COMPRAS POR ORDER_ID ---\n");
    for (int i = 0; i < 5; i++) {
        printf("\n>> Busca #%d - Order ID: %lld\n", i+1, orders_teste[i]);
        printf("\n[BUSCA POR INDICE INVERSO EM ARQUIVO]\n");
        double inicio = get_time_ms();
        consultar_compra_por_id(arq_compras, idx_compras, orders_teste[i]);
        double fim = get_time_ms();
        printf("  TEMPO: %.4f ms\n", fim - inicio);
        consultar_compra_hash_memoria(arq_compras, orders_teste[i]);
    }
}

void teste_insercao_comparativo(const char *arq_produtos, const char *arq_compras) {
    printf("\n========================================\n");
    printf("   TESTE COMPARATIVO DE INSERCAO\n");
    printf("========================================\n");
    
    printf("\n--- TESTE: INSERCAO DE PRODUTO ---\n");
    
    Produto novo_produto;
    memset(&novo_produto, 0, sizeof(Produto));
    novo_produto.product_id = 9999999999999;
    novo_produto.price = 99.99;
    novo_produto.category_id = 1234567890LL;
    strcpy(novo_produto.category_alias, "Test Category");
    novo_produto.brand_id = 999;
    novo_produto.gender = 'U';
    novo_produto.ativo = 1;
    
    printf("\nMÃ©todo Arquivo:\n");
    double inicio = get_time_ms();
    
    FILE *fp = fopen(arq_produtos, "rb");
    FILE *temp_file = fopen("temp_produtos.dat", "wb");
    
    if (!fp || !temp_file) {
        printf("Erro ao abrir arquivos!\n");
        if (fp) fclose(fp);
        if (temp_file) fclose(temp_file);
        return;
    }
    
    Produto p;
    char newline;
    int inserido = 0;
    
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        if (!inserido && novo_produto.product_id < p.product_id) {
            fwrite(&novo_produto, sizeof(Produto), 1, temp_file);
            fwrite("\n", 1, 1, temp_file);
            inserido = 1;
        }
        
        fwrite(&p, sizeof(Produto), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    if (!inserido) {
        fwrite(&novo_produto, sizeof(Produto), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    fclose(fp);
    fclose(temp_file);
    
    remove(arq_produtos);
    rename("temp_produtos.dat", arq_produtos);
    
    criar_indice_produtos(arq_produtos, "produtos.idx");
    
    double tempo_arquivo_produto = get_time_ms() - inicio;
    printf("  Tempo: %.4f ms\n", tempo_arquivo_produto);
    
    printf("\nMetodo Memoria:\n");
    
    if (arvore_produtos == NULL) {
        criar_indice_arvore_produtos(arq_produtos);
    }
    
    Produto novo_produto2;
    memset(&novo_produto2, 0, sizeof(Produto));
    novo_produto2.product_id = 9999999999998LL;
    novo_produto2.price = 88.88;
    novo_produto2.category_id = 1234567891LL;
    strcpy(novo_produto2.category_alias, "Test Category 2");
    novo_produto2.brand_id = 998;
    novo_produto2.gender = 'M';
    novo_produto2.ativo = 1;
    
    inicio = get_time_ms();
    
    fp = fopen(arq_produtos, "rb");
    temp_file = fopen("temp_produtos.dat", "wb");
    
    if (!fp || !temp_file) {
        printf("Erro ao abrir arquivos!\n");
        if (fp) fclose(fp);
        if (temp_file) fclose(temp_file);
        return;
    }
    
    inserido = 0;
    long posicao_inserida = 0;
    
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        if (!inserido && novo_produto2.product_id < p.product_id) {
            posicao_inserida = ftell(temp_file);
            fwrite(&novo_produto2, sizeof(Produto), 1, temp_file);
            fwrite("\n", 1, 1, temp_file);
            inserido = 1;
        }
        
        fwrite(&p, sizeof(Produto), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    if (!inserido) {
        posicao_inserida = ftell(temp_file);
        fwrite(&novo_produto2, sizeof(Produto), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    fclose(fp);
    fclose(temp_file);
    
    remove(arq_produtos);
    rename("temp_produtos.dat", arq_produtos);
    
    inserir_arvore_b(arvore_produtos, novo_produto2.product_id, posicao_inserida);
    
    double tempo_memoria_produto = get_time_ms() - inicio;
    printf("  Tempo: %.4f ms\n", tempo_memoria_produto);
    
    printf("\n>>> RESULTADO PRODUTOS:\n");
    printf("    Arquivo:  %.4f ms\n", tempo_arquivo_produto);
    printf("    Memoria:  %.4f ms\n", tempo_memoria_produto);
    
    printf("\n--- TESTE: INSERÃ‡ÃƒO DE COMPRA ---\n");
    
    Compra nova_compra;
    memset(&nova_compra, 0, sizeof(Compra));
    strcpy(nova_compra.order_datetime, "2024-12-25 10:30:00");
    nova_compra.order_id = 9999999999999LL;
    nova_compra.product_id = 1515489LL;
    nova_compra.quantity = 5;
    nova_compra.user_id = 8888888888LL;
    nova_compra.ativo = 1;
    
    printf("\nMetodo Arquivo:\n");
    printf("  Inserindo: Order=%lld, Product=%lld\n", nova_compra.order_id, nova_compra.product_id);
    
    inicio = get_time_ms();
    
    fp = fopen(arq_compras, "rb");
    temp_file = fopen("temp_compras.dat", "wb");
    
    if (!fp || !temp_file) {
        printf("Erro ao abrir arquivos!\n");
        if (fp) fclose(fp);
        if (temp_file) fclose(temp_file);
        return;
    }
    
    Compra c;
    inserido = 0;
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        if (!inserido && nova_compra.order_id < c.order_id) {
            fwrite(&nova_compra, sizeof(Compra), 1, temp_file);
            fwrite("\n", 1, 1, temp_file);
            inserido = 1;
        }
        
        fwrite(&c, sizeof(Compra), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    if (!inserido) {
        fwrite(&nova_compra, sizeof(Compra), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    fclose(fp);
    fclose(temp_file);
    
    remove(arq_compras);
    rename("temp_compras.dat", arq_compras);
    
    criar_indice_inverso_compras(arq_compras, "compras_inv.idx");
    
    double tempo_arquivo_compra = get_time_ms() - inicio;
    printf("  Tempo: %.4f ms\n", tempo_arquivo_compra);
    
    printf("\nMÃ©todo MemÃ³ria:\n");
    
    if (hash_compras == NULL) {
        criar_indice_hash_compras(arq_compras);
    }
    
    Compra nova_compra2;
    memset(&nova_compra2, 0, sizeof(Compra));
    strcpy(nova_compra2.order_datetime, "2024-12-25 11:45:00");
    nova_compra2.order_id = 9999999999998LL;
    nova_compra2.product_id = 1515490LL;
    nova_compra2.quantity = 3;
    nova_compra2.user_id = 7777777777LL;
    nova_compra2.ativo = 1;
    
    printf("  Inserindo: Order=%lld, Product=%lld\n", nova_compra2.order_id, nova_compra2.product_id);
    
    inicio = get_time_ms();
    
    fp = fopen(arq_compras, "rb");
    temp_file = fopen("temp_compras.dat", "wb");
    
    if (!fp || !temp_file) {
        printf("Erro ao abrir arquivos!\n");
        if (fp) fclose(fp);
        if (temp_file) fclose(temp_file);
        return;
    }
    
    inserido = 0;
    posicao_inserida = 0;
    
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        if (!inserido && nova_compra2.order_id < c.order_id) {
            posicao_inserida = ftell(temp_file);
            fwrite(&nova_compra2, sizeof(Compra), 1, temp_file);
            fwrite("\n", 1, 1, temp_file);
            inserido = 1;
        }
        
        fwrite(&c, sizeof(Compra), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    if (!inserido) {
        posicao_inserida = ftell(temp_file);
        fwrite(&nova_compra2, sizeof(Compra), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    fclose(fp);
    fclose(temp_file);
    
    remove(arq_compras);
    rename("temp_compras.dat", arq_compras);
    
    inserir_hash(hash_compras, nova_compra2.order_id, posicao_inserida);
    
    double tempo_memoria_compra = get_time_ms() - inicio;
    printf("  Tempo: %.4f ms\n", tempo_memoria_compra);
    
    printf("\n>>> RESULTADO COMPRAS:\n");
    printf("    Arquivo:  %.4f ms\n", tempo_arquivo_compra);
    printf("    Memoria:  %.4f ms\n", tempo_memoria_compra);
    
}

void menu() {
    printf("\n========================================\n");
    printf("   SISTEMA DE LOJA DE JOIAS ONLINE\n");
    printf("========================================\n");
    printf("ARQUIVOS E INDICES:\n");
    printf("1. Criar/Recriar arquivos de dados\n");
    printf("2. Criar indices\n");
    printf("\nCONSULTAS:\n");
    printf("3. Mostrar produtos\n");
    printf("4. Mostrar compras\n");
    printf("5. Consultar produto por ID\n");
    printf("6. Consultar compra por ID\n");
    printf("\nPRODUTOS:\n");
    printf("7. Inserir novo produto\n");
    printf("8. Deletar produto\n");
    printf("9. Reorganizar arquivo de produtos\n");
    printf("\nCOMPRAS:\n");
    printf("10. Inserir nova compra/item\n");
    printf("11. Deletar item de compra\n");
    printf("12. Deletar compra completa\n");
    printf("13. Reorganizar arquivo de compras\n");
    printf("\nINDICES MEMORIA:\n");
    printf("14. Consultar indice em memoria de produtos\n");
    printf("15. Consultar indice em memoria de compras\n");
    printf("\nTestes comparativos:\n");
    printf("16. Executar teste de comparacao com ids pre-selecionados\n");
    printf("17. Executar teste de comparacao (remover)\n");
	printf("18. Teste de comparacao (insercao)\n");
    printf("\n0. Sair\n");
    printf("========================================\n");
    printf("Escolha: ");
}

int main() {
    int opcao;
    long long id, id2;
    
    char arq_produtos[] = "produtos.dat";
    char arq_compras[] = "compras.dat";
    char idx_produtos[] = "produtos.idx";
    char idx_compras[] = "compras_inv.idx";
    carregar_indices_automatico(idx_produtos, idx_compras);
    
    do {
        menu();
        scanf("%d", &opcao);
        limpar_buffer();
        
        switch(opcao) {
            case 1:
                printf("\nCriando arquivos de dados...\n");
                inserir_produtos(arq_produtos);
                inserir_compras(arq_compras);
                break;
                
            case 2:
                printf("\nCriando indices...\n");
                criar_indice_produtos(arq_produtos, idx_produtos);
                criar_indice_inverso_compras(arq_compras, idx_compras);
                criar_indice_arvore_produtos(arq_produtos);
                criar_indice_hash_compras(arq_compras);
                break;
                
            case 3:
                mostrar_produtos(arq_produtos);
                break;
                
            case 4:
                mostrar_compras(arq_compras);
                break;
                
            case 5:
                printf("\nDigite o Product ID: ");
                scanf("%lld", &id);
                limpar_buffer();
                consultar_produto(arq_produtos, idx_produtos, id);
                break;
                
            case 6:
                printf("\nDigite o Order ID: ");
                scanf("%lld", &id);
                limpar_buffer();
                consultar_compra_por_id(arq_compras, idx_compras, id);
                break;
                
            case 7:
                inserir_produto_individual(arq_produtos, idx_produtos);
                criar_indice_produtos(arq_produtos, idx_produtos);
                break;
                
            case 8:
                printf("\nDigite o Product ID para deletar: ");
                scanf("%lld", &id);
                limpar_buffer();
                deletar_produto(arq_produtos, idx_produtos, id);
                deletar_produto_com_indice(arq_produtos, id);
                break;
                
            case 9:
                printf("\nDeseja reorganizar o arquivo? Isso removera permanentemente os produtos deletados.\n");
                printf("Continuar? (S/N): ");
                char confirma;
                scanf("%c", &confirma);
                limpar_buffer();
                if (confirma == 'S' || confirma == 's') {
                    reorganizar_produtos(arq_produtos, idx_produtos);
                    criar_indice_produtos(arq_produtos, idx_produtos);
                } else {
                    printf("Operacao cancelada.\n");
                }
                break;
                
            case 10:
                inserir_compra_individual(arq_compras, idx_compras);
                criar_indice_inverso_compras(arq_compras, idx_compras);
                break;
                
            case 11:
                printf("\nDigite o Order ID: ");
                scanf("%lld", &id);
                limpar_buffer();
                printf("Digite o Product ID do item a deletar: ");
                scanf("%lld", &id2);
                limpar_buffer();
                deletar_item_compra(arq_compras, idx_compras, id, id2);
                deletar_compra_com_indice(arq_compras, id, id2);
                break;
                
            case 12:
                printf("\nDigite o Order ID para deletar todos os itens: ");
                scanf("%lld", &id);
                limpar_buffer();
                printf("Deseja deletar TODOS os itens desta compra? (S/N): ");
                char confirma2;
                scanf("%c", &confirma2);
                limpar_buffer();
                if (confirma2 == 'S' || confirma2 == 's') {
                    deletar_compra_completa(arq_compras, idx_compras, id);
                    deletar_compra_completa_com_indice(arq_compras, id);
                } else {
                    printf("Operação cancelada.\n");
                }
                break;
                
            case 13:
                printf("\nDeseja reorganizar o arquivo de compras? Isso removerá permanentemente os itens deletados.\n");
                printf("Continuar? (S/N): ");
                char confirma3;
                scanf("%c", &confirma3);
                limpar_buffer();
                if (confirma3 == 'S' || confirma3 == 's') {
                    reorganizar_compras(arq_compras, idx_compras);
                    criar_indice_inverso_compras(arq_compras, idx_compras);
                } else {
                    printf("Operacao cancelada.\n");
                }
                break;          
            case 14:
                printf("\nDigite o Product ID: ");
                scanf("%lld", &id);
                limpar_buffer();
                consultar_produto_memoria(arq_produtos, id);
                break;
                
            case 15:
                printf("\nDigite o User ID: ");
                scanf("%lld", &id);
                limpar_buffer();
                consultar_compra_hash_memoria(arq_compras, id);
                break;       
            case 16:
                executar_testes_comparativos(arq_produtos, idx_produtos, arq_compras, idx_compras);
                break;
                
            case 17:
                teste_remocao_comparativo(arq_produtos, arq_compras);
                break;
            case 18:
			    teste_insercao_comparativo(arq_produtos, arq_compras);
			    break;
                
            case 0:
                printf("\nEncerrando...\n");
                if (arvore_produtos != NULL) {
                    liberar_arvore_b(arvore_produtos->raiz);
                    free(arvore_produtos);
                }
                if (hash_compras != NULL) {
                    liberar_tabela_hash(hash_compras);
                }
                break;
                
            default:
                printf("\nOpção invalida!\n");
        }
        
    } while(opcao != 0);
    
    return 0;
}
