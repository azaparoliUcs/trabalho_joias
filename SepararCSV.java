package br.com.ucs.laboratorio.gestao.infrastructure;

import java.io.*;
import java.util.*;

public class SepararCSV {

    public static void main(String[] args) {
        String arquivoOriginal = "jewelry.csv";
        String arquivoProdutos = "produtos.csv";
        String arquivoCompras = "compras.csv";

        try {
            processarCSV(arquivoOriginal, arquivoProdutos, arquivoCompras);

            System.out.println("\nIniciando ordenação externa...");

            externalMergeSort(arquivoProdutos, "Product ID");
            externalMergeSort(arquivoCompras, "Order ID");

            System.out.println("✓ Arquivos ordenados com sucesso!");
        } catch (IOException e) {
            System.err.println("Erro ao processar arquivo: " + e.getMessage());
            e.printStackTrace();
        }
    }

    // ======================================================
    // == PROCESSAMENTO INICIAL (Separação dos CSVs)
    // ======================================================
    public static void processarCSV(String arquivoOriginal, String arquivoProdutos,
                                    String arquivoCompras) throws IOException {

        BufferedReader br = new BufferedReader(new FileReader(arquivoOriginal));
        String cabecalho = br.readLine();
        String[] colunas = cabecalho.split(",");
        Map<String, Integer> indicesColunas = mapearColunas(colunas);

        Map<String, Produto> produtosMap = new LinkedHashMap<>();
        List<Compra> compras = new ArrayList<>();

        String linha;
        while ((linha = br.readLine()) != null) {
            String[] valores = parsearLinha(linha);

            String productId = valores[indicesColunas.get("Purchased product ID")];
            String priceUSD = valores[indicesColunas.get("Price in USD")];
            String categoryId = valores[indicesColunas.get("Category ID")];
            String categoryAlias = valores[indicesColunas.get("Category alias")];
            String brandId = valores[indicesColunas.get("Brand ID")];
            String productGender = valores[indicesColunas.get("Product gender")];

            if (productGender == null || productGender.trim().isEmpty()) {
                productGender = "u";
            }

            produtosMap.putIfAbsent(productId, new Produto(productId, priceUSD,
                    categoryId, categoryAlias, brandId, productGender));

            String orderDatetime = valores[indicesColunas.get("Order datetime")];
            String orderId = valores[indicesColunas.get("Order ID")];
            String quantity = valores[indicesColunas.get("Quantity of SKU in the order")];
            String userId = valores[indicesColunas.get("User ID")];

            compras.add(new Compra(orderDatetime, orderId, productId, quantity, userId));
        }
        br.close();

        escreverProdutos(arquivoProdutos, produtosMap.values());
        System.out.println("✓ Arquivo '" + arquivoProdutos + "' criado com " +
                produtosMap.size() + " produtos únicos");

        escreverCompras(arquivoCompras, compras);
        System.out.println("✓ Arquivo '" + arquivoCompras + "' criado com " +
                compras.size() + " registros de compras");
    }

    private static Map<String, Integer> mapearColunas(String[] colunas) {
        Map<String, Integer> mapa = new HashMap<>();
        for (int i = 0; i < colunas.length; i++) {
            mapa.put(colunas[i].trim(), i);
        }
        return mapa;
    }

    private static String[] parsearLinha(String linha) {
        List<String> valores = new ArrayList<>();
        StringBuilder campo = new StringBuilder();
        boolean dentroAspas = false;

        for (char c : linha.toCharArray()) {
            if (c == '"') {
                dentroAspas = !dentroAspas;
            } else if (c == ',' && !dentroAspas) {
                valores.add(campo.toString().trim());
                campo = new StringBuilder();
            } else {
                campo.append(c);
            }
        }
        valores.add(campo.toString().trim());
        return valores.toArray(new String[0]);
    }

    private static void escreverProdutos(String arquivo, Collection<Produto> produtos)
            throws IOException {
        BufferedWriter bw = new BufferedWriter(new FileWriter(arquivo));
        bw.write("Product ID,Price in USD,Category ID,Category alias,Brand ID,Product gender\n");
        for (Produto p : produtos) {
            bw.write(String.format("%s,%s,%s,%s,%s,%s\n",
                    p.productId, p.priceUSD, p.categoryId,
                    p.categoryAlias, p.brandId, p.productGender));
        }
        bw.close();
    }

    private static void escreverCompras(String arquivo, List<Compra> compras)
            throws IOException {
        BufferedWriter bw = new BufferedWriter(new FileWriter(arquivo));
        bw.write("Order datetime,Order ID,Purchased product ID,Quantity of SKU in the order,User ID\n");
        for (Compra c : compras) {
            bw.write(String.format("%s,%s,%s,%s,%s\n",
                    c.orderDatetime, c.orderId, c.productId, c.quantity, c.userId));
        }
        bw.close();
    }

    // ======================================================
    // == ORDENAÇÃO EXTERNA (External Merge Sort)
    // ======================================================
    public static void externalMergeSort(String arquivo, String chave) throws IOException {
        int TAMANHO_BLOCO = 50000; // Ajuste conforme a memória disponível
        String cabecalho;
        List<File> arquivosParciais = new ArrayList<>();

        try (BufferedReader reader = new BufferedReader(new FileReader(arquivo))) {
            cabecalho = reader.readLine();

            List<String> buffer = new ArrayList<>(TAMANHO_BLOCO);
            String linha;
            while ((linha = reader.readLine()) != null) {
                buffer.add(linha);
                if (buffer.size() >= TAMANHO_BLOCO) {
                    arquivosParciais.add(ordenarEGravarBloco(buffer, cabecalho, chave));
                    buffer.clear();
                }
            }

            if (!buffer.isEmpty()) {
                arquivosParciais.add(ordenarEGravarBloco(buffer, cabecalho, chave));
            }
        }

        mergeArquivos(arquivosParciais, new File(arquivo), cabecalho, chave);

        for (File f : arquivosParciais) f.delete();
    }

    private static File ordenarEGravarBloco(List<String> linhas, String cabecalho, String chave) throws IOException {
        Comparator<String> comparator = getComparator(cabecalho, chave);
        linhas.sort(comparator);
        File temp = File.createTempFile("bloco_", ".csv");
        try (BufferedWriter bw = new BufferedWriter(new FileWriter(temp))) {
            for (String l : linhas) bw.write(l + "\n");
        }
        return temp;
    }

    private static void mergeArquivos(List<File> arquivos, File arquivoSaida,
                                      String cabecalho, String chave) throws IOException {
        PriorityQueue<LinhaArquivo> fila = new PriorityQueue<>(
                Comparator.comparing(l -> l.linha, getComparator(cabecalho, chave))
        );

        List<BufferedReader> readers = new ArrayList<>();
        for (File f : arquivos) {
            BufferedReader br = new BufferedReader(new FileReader(f));
            readers.add(br);
            String linha = br.readLine();
            if (linha != null) fila.add(new LinhaArquivo(linha, br));
        }

        try (BufferedWriter bw = new BufferedWriter(new FileWriter(arquivoSaida))) {
            bw.write(cabecalho + "\n");
            while (!fila.isEmpty()) {
                LinhaArquivo menor = fila.poll();
                bw.write(menor.linha + "\n");
                String proxima = menor.reader.readLine();
                if (proxima != null) fila.add(new LinhaArquivo(proxima, menor.reader));
            }
        }

        for (BufferedReader br : readers) br.close();
    }

    private static Comparator<String> getComparator(String cabecalho, String chave) {
        String[] cols = cabecalho.split(",");
        int idx = -1;
        for (int i = 0; i < cols.length; i++) {
            if (cols[i].trim().equalsIgnoreCase(chave)) {
                idx = i;
                break;
            }
        }
        final int index = idx;
        return Comparator.comparing(l -> l.split(",")[index]);
    }

    private static class LinhaArquivo {
        String linha;
        BufferedReader reader;
        LinhaArquivo(String linha, BufferedReader reader) {
            this.linha = linha;
            this.reader = reader;
        }
    }

    // ======================================================
    // == CLASSES AUXILIARES
    // ======================================================
    static class Produto {
        String productId, priceUSD, categoryId, categoryAlias, brandId, productGender;
        Produto(String productId, String priceUSD, String categoryId,
                String categoryAlias, String brandId, String productGender) {
            this.productId = productId;
            this.priceUSD = priceUSD;
            this.categoryId = categoryId;
            this.categoryAlias = categoryAlias;
            this.brandId = brandId;
            this.productGender = productGender;
        }
    }

    static class Compra {
        String orderDatetime, orderId, productId, quantity, userId;
        Compra(String orderDatetime, String orderId, String productId, String quantity, String userId) {
            this.orderDatetime = orderDatetime;
            this.orderId = orderId;
            this.productId = productId;
            this.quantity = quantity;
            this.userId = userId;
        }
    }
}
