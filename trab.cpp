#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#define MAX_STRING 100
#define BLOCO_INDICE 3

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

// ========== FUNÇÕES AUXILIARES ==========
int indices_carregados = 0;

void limpar_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Verificar se arquivo existe
int arquivo_existe(const char *nome) {
    FILE *f = fopen(nome, "rb");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

// Carregar índices existentes ao iniciar
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

// Inserir dados de produtos no arquivo (lendo do CSV JÁ ORDENADO)
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
    
    // Pular cabeçalho
    fgets(linha, sizeof(linha), csv);
    
    printf("\nLendo produtos do CSV...\n");
    
    // Ler dados do CSV e gravar diretamente no arquivo binário
    while (fgets(linha, sizeof(linha), csv)) {
        Produto p;
        
        // Inicializar registro com zeros
        memset(&p, 0, sizeof(Produto));
        p.ativo = 1;  // Produto ativo por padrão
        p.gender = 'u';  // Default: unisex
        
        // Parse manual do CSV para lidar com campos vazios
        char *token;
        char linha_copia[500];
        strcpy(linha_copia, linha);
        
        int campo = 0;
        char *ptr = linha_copia;
        char *inicio = ptr;
        
        while (*ptr && campo < 6) {
            if (*ptr == ',' || *ptr == '\n' || *ptr == '\r') {
                *ptr = '\0';  // Termina o campo atual
                
                switch(campo) {
                    case 0:  // Product ID
                        if (strlen(inicio) > 0) {
                            sscanf(inicio, "%lld", &p.product_id);
                        }
                        break;
                    case 1:  // Price
                        if (strlen(inicio) > 0) {
                            sscanf(inicio, "%f", &p.price);
                        }
                        break;
                    case 2:  // Category ID
                        if (strlen(inicio) > 0) {
                            sscanf(inicio, "%lld", &p.category_id);
                        }
                        break;
                    case 3:  // Category alias
                        if (strlen(inicio) > 0) {
                            strncpy(p.category_alias, inicio, 49);
                            p.category_alias[49] = '\0';
                        }
                        break;
                    case 4:  // Brand ID
                        if (strlen(inicio) > 0) {
                            sscanf(inicio, "%d", &p.brand_id);
                        }
                        break;
                    case 5:  // Gender
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
        
        // Gravar registro no arquivo binário
        if (p.product_id > 0) {  // Só grava se leu o product_id
            fwrite(&p, sizeof(Produto), 1, fp);
            fwrite("\n", 1, 1, fp);
            count++;
            
            // Debug: mostrar primeiros 5 registros
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

// Criar índice parcial para produtos (em níveis)
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
    
    // Criar índice parcial a cada BLOCO_INDICE registros
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

// Pesquisa binária no índice de produtos
long pesquisa_binaria_indice(const char *arq_indice, long long chave, long *pos_inicial) {
    FILE *fi = fopen(arq_indice, "rb");
    if (!fi) {
        *pos_inicial = 0;
        return -1;
    }
    
    // Contar entradas no índice
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
    
    // Retorna a última posição menor ou igual à chave
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

// Consultar produto usando índice parcial
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
    
    // Buscar sequencialmente a partir da posição do índice
    while (registros_lidos < BLOCO_INDICE * 2) {
        if (fread(&p, sizeof(Produto), 1, fp) != 1) break;
        if (fread(&newline, 1, 1, fp) != 1) break;
        
        registros_lidos++;
        
        // Verificar se encontrou o produto
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
        
        // Se passou do ID procurado, pode parar (arquivo ordenado)
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

// Inserir um novo produto individual
void inserir_produto_individual(const char *arq_dados, const char *arq_indice) {
    Produto novo;
    memset(&novo, 0, sizeof(Produto));
    
    printf("\n=== INSERIR NOVO PRODUTO ===\n");
    printf("Product ID: ");
    scanf("%lld", &novo.product_id);
    limpar_buffer();
    
    // Verificar se já existe
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
    
    novo.ativo = 1;  // Produto ativo
    
    // Encontrar posição correta para inserção (manter ordenação)
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
    
    // Copiar produtos mantendo ordenação
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        // Inserir novo produto na posição correta
        if (!inserido && novo.product_id < p.product_id) {
            fwrite(&novo, sizeof(Produto), 1, temp_file);
            fwrite("\n", 1, 1, temp_file);
            inserido = 1;
        }
        
        fwrite(&p, sizeof(Produto), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    // Se não foi inserido, adiciona no final
    if (!inserido) {
        fwrite(&novo, sizeof(Produto), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    fclose(fp);
    fclose(temp_file);
    
    // Substituir arquivo original
    remove(arq_dados);
    rename("temp_produtos.dat", arq_dados);
    
    printf("\nProduto inserido com sucesso!\n");
}

// Deletar produto (marcação lógica)
void deletar_produto(const char *arq_dados, const char *arq_indice, long long product_id) {
    FILE *fp = fopen(arq_dados, "r+b");  // Modo leitura e escrita
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
        posicao = ftell(fp) - sizeof(Produto);  // Posição do registro lido
        fread(&newline, 1, 1, fp);
        
        if (p.product_id == product_id) {
            if (!p.ativo) {
                printf("Produto %lld já está deletado.\n", product_id);
                fclose(fp);
                return;
            }
            
            // Marcar como inativo
            p.ativo = 0;
            
            // Voltar para a posição e reescrever
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

// Reorganizar arquivo (compactar removendo deletados)
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
    
    // Copiar apenas produtos ativos
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
    
    // Substituir arquivo original
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
    
    // Remover \n e \r do final
    char *newline_pos = strchr(linha, '\n');
    if (newline_pos) *newline_pos = '\0';
    newline_pos = strchr(linha, '\r');
    if (newline_pos) *newline_pos = '\0';
    
    // Separar campos manualmente
    char *campo1 = linha;  // datetime
    char *campo2 = NULL;   // order_id
    char *campo3 = NULL;   // product_id
    char *campo4 = NULL;   // quantity
    char *campo5 = NULL;   // user_id
    
    int virgula_count = 0;
    char *ptr = linha;
    
    while (*ptr) {
        if (*ptr == ',') {
            *ptr = '\0';  // Substitui vírgula por \0
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
    
    // Validar se temos todos os campos
    if (virgula_count != 4 || !campo2 || !campo3 || !campo4 || !campo5) {
        return 0;
    }
    
    // Processar cada campo
    // 1. Order datetime
    if (strlen(campo1) > 0) {
        strncpy(c->order_datetime, campo1, 29);
        c->order_datetime[29] = '\0';
    }
    
    // 2. Order ID - IMPORTANTE: usar %lld
    if (strlen(campo2) > 0) {
        if (sscanf(campo2, "%lld", &c->order_id) != 1) {
            return 0;
        }
    } else {
        return 0;
    }
    
    // 3. Product ID
    if (strlen(campo3) > 0) {
        if (sscanf(campo3, "%lld", &c->product_id) != 1) {
            return 0;
        }
    }
    
    // 4. Quantity
    if (strlen(campo4) > 0) {
        if (sscanf(campo4, "%d", &c->quantity) != 1) {
            return 0;
        }
    }
    
    // 5. User ID
    if (strlen(campo5) > 0) {
        if (sscanf(campo5, "%lld", &c->user_id) != 1) {
            return 0;
        }
    }
    
    return 1;
}

// Inserir dados de compras no arquivo (lendo do CSV JÁ ORDENADO)
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
    
    // Pular cabeçalho
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
            
            // Debug: primeiros 5 registros
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

// Mostrar compras
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
        if (c.ativo) {  // Mostra apenas compras ativas
            printf("%-30s | %-20lld | %-20lld | %3d | %-12lld\n", 
                   c.order_datetime, c.order_id, c.product_id, c.quantity, c.user_id);
            i++;
        }
    }
    
    printf("Total: %d compras\n\n", i);
    fclose(fp);
}

// Criar índice inverso para compras (por order_id - pedidos)
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
    
    // Reserva espaço para escrever o número de pedidos depois
    int num_pedidos = 0;
    fwrite(&num_pedidos, sizeof(int), 1, fi);
    
    Compra c;
    char newline;
    
    // Variáveis para o pedido atual
    long long order_id_atual = -1;
    long posicoes_temp[50];  // Buffer: 50 itens max por pedido
    int count_itens = 0;
    int total_registros = 0;
    
    // UMA ÚNICA PASSADA pelo arquivo
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        // Ler o newline
        fread(&newline, 1, 1, fp);
        
        // Calcular posição ANTES de ler o registro
        long pos = ftell(fp) - sizeof(Compra) - 1;
        
        total_registros++;
        
        // Debug: primeiros 5 registros
        if (total_registros <= 5) {
            printf("  Lendo reg %d: Order=%lld, Pos=%ld, Ativo=%d\n", 
                   total_registros, c.order_id, pos, c.ativo);
        }
        
        if (c.ativo) {
            // Detecta mudança de order_id (novo pedido)
            if (c.order_id != order_id_atual) {
                
                // Grava o pedido anterior (se existir)
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
                
                // Inicia novo pedido
                order_id_atual = c.order_id;
                count_itens = 0;
            }
            
            // Adiciona item ao pedido atual
            if (count_itens < 50) {
                posicoes_temp[count_itens++] = pos;
            } else {
                printf("[AVISO] Pedido %lld excedeu limite de 50 itens!\n", 
                       order_id_atual);
            }
        }
    }
    
    // Grava o ÚLTIMO pedido
    if (order_id_atual != -1 && count_itens > 0) {
        fwrite(&order_id_atual, sizeof(long long), 1, fi);
        fwrite(&count_itens, sizeof(int), 1, fi);
        fwrite(posicoes_temp, sizeof(long), count_itens, fi);
        num_pedidos++;
        printf("  -> Gravando último pedido: Order=%lld, Itens=%d\n", 
               order_id_atual, count_itens);
    }
    
    // Volta ao início e grava o número total de pedidos
    fseek(fi, 0, SEEK_SET);
    fwrite(&num_pedidos, sizeof(int), 1, fi);
    
    fclose(fp);
    fclose(fi);
    
    printf("\n[SUCESSO] Índice inverso criado:\n");
    printf("  - Total de registros lidos: %d\n", total_registros);
    printf("  - Total de pedidos únicos: %d\n", num_pedidos);
}

// NOVA: Busca binária no índice inverso
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
    
    // BUSCA SEQUENCIAL (por enquanto, para garantir que funciona)
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
            
            // Ler as posições
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

// Consultar compra por order_id usando índice inverso
void consultar_compra_por_id(const char *arq_dados, const char *arq_indice, long long order_id) {
    int num_itens;
    long *posicoes;
    
    // Busca usando a função corrigida
    int resultado = busca_binaria_indice_inverso(arq_indice, order_id, &num_itens, &posicoes);
    
    if (resultado == -1) {
        printf("Compra %lld não encontrada.\n", order_id);
        return;
    }
    
    // Abrir arquivo de dados e ler os itens
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
    
    // Ler todos os itens do pedido
    for (int i = 0; i < num_itens; i++) {
        fseek(fp, posicoes[i], SEEK_SET);
        Compra c;
        
        if (fread(&c, sizeof(Compra), 1, fp) != 1) {
            printf("[ERRO] Falha ao ler item %d na posição %ld\n", i, posicoes[i]);
            continue;
        }
        fread(&newline, 1, 1, fp);
        
        if (i < 3) {  // Debug primeiros 3
            printf("[DEBUG] Item %d: Order=%lld, Product=%lld, Ativo=%d\n", 
                   i+1, c.order_id, c.product_id, c.ativo);
        }
        
        if (c.ativo) {
            itens[itens_ativos++] = c;
        }
    }
    
    fclose(fp);
    free(posicoes);
    
    // Exibir resultado
    if (itens_ativos == 0) {
        printf("Compra %lld foi deletada (todos os itens inativos).\n", order_id);
        return;
    }
    
    if (itens_ativos == 1) {
        // Um único item
        printf("\n=== COMPRA ENCONTRADA ===\n");
        printf("Order ID: %lld\n", itens[0].order_id);
        printf("Data/Hora: %s\n", itens[0].order_datetime);
        printf("Product ID: %lld\n", itens[0].product_id);
        printf("Quantidade: %d\n", itens[0].quantity);
        printf("User ID: %lld\n", itens[0].user_id);
    } else {
        // Múltiplos itens
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

// Inserir uma nova compra/item individual
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
    
    nova.ativo = 1;  // Compra ativa
    
    // Encontrar posição correta para inserção (manter ordenação por order_id)
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
    
    // Copiar compras mantendo ordenação
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        fread(&newline, 1, 1, fp);
        
        // Inserir nova compra na posição correta
        if (!inserido && nova.order_id < c.order_id) {
            fwrite(&nova, sizeof(Compra), 1, temp_file);
            fwrite("\n", 1, 1, temp_file);
            inserido = 1;
        }
        
        fwrite(&c, sizeof(Compra), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    // Se não foi inserido, adiciona no final
    if (!inserido) {
        fwrite(&nova, sizeof(Compra), 1, temp_file);
        fwrite("\n", 1, 1, temp_file);
    }
    
    fclose(fp);
    fclose(temp_file);
    
    // Substituir arquivo original
    remove(arq_dados);
    rename("temp_compras.dat", arq_dados);
    
    printf("\nCompra/Item inserido com sucesso!\n");
}

// Deletar item específico de uma compra
void deletar_item_compra(const char *arq_dados, const char *arq_indice, long long order_id, long long product_id) {
    FILE *fp = fopen(arq_dados, "r+b");  // Modo leitura e escrita
    if (!fp) {
        printf("Erro ao abrir arquivo de compras!\n");
        return;
    }
    
    Compra c;
    char newline;
    int encontrado = 0;
    long posicao;
    
    // Buscar o item específico
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        posicao = ftell(fp) - sizeof(Compra);  // Posição do registro lido
        fread(&newline, 1, 1, fp);
        
        if (c.order_id == order_id && c.product_id == product_id) {
            if (!c.ativo) {
                printf("Este item ja esta deletado.\n");
                fclose(fp);
                return;
            }
            
            // Marcar como inativo
            c.ativo = 0;
            
            // Voltar para a posição e reescrever
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

// Deletar todos os itens de uma compra (order_id inteiro)
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
    
    // Buscar todos os itens deste order_id
    while (fread(&c, sizeof(Compra), 1, fp) == 1) {
        posicao = ftell(fp) - sizeof(Compra);
        fread(&newline, 1, 1, fp);
        
        if (c.order_id == order_id && c.ativo) {
            // Marcar como inativo
            c.ativo = 0;
            
            // Voltar e reescrever
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

// Reorganizar arquivo de compras (compactar removendo deletados)
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
    
    // Copiar apenas compras ativas
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
    
    // Substituir arquivo original
    remove(arq_dados);
    rename("temp_compras.dat", arq_dados);
    
    printf("\n=== REORGANIZACAO DE COMPRAS CONCLUIDA ===\n");
    printf("Itens ativos mantidos: %d\n", ativos);
    printf("Itens deletados removidos: %d\n", deletados);
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
                
            case 0:
                printf("\nEncerrando programa...\n");
                break;
                
            default:
                printf("\nOpção invalida!\n");
        }
        
    } while(opcao != 0);
    
    return 0;
}
