#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdio>

using namespace std;

const int batch_size = 128 * 1024 * 1024;
const int num_threads = thread::hardware_concurrency();



unordered_map<string, int> dic{
    {"the", 0}, {"be", 1}, {"to", 2}, {"of", 3}, {"and", 4},
    {"a", 5}, {"in", 6}, {"that", 7}, {"have", 8}, {"i", 9},
    {"it", 10}, {"for", 11}, {"not", 12}, {"on", 13}, {"with", 14},
    {"he", 15}, {"as", 16}, {"you", 17}, {"do", 18}, {"at", 19}
};
int tam_dic=dic.size();


vector<long long> conteo_global(tam_dic, 0);
mutex mtx;
vector<FILE*> archivos_globales(tam_dic);

void cont_palabras(string texto, long long offset_inicio, vector<vector<long long>>& local_pos) {
    vector<long long> conteo_local(tam_dic, 0);
    string palabra;
    long long pos_en_texto = 0;
    for (char c : texto) {
        if (c == ' ') {
            if (!palabra.empty()) {
                auto it = dic.find(palabra);
                if (it != dic.end()) {
                    int idx = it->second;
                    conteo_local[idx]++;
                    local_pos[idx].push_back(offset_inicio + pos_en_texto - palabra.size());
                }
            }
            palabra.clear();
        } else {
            if (c >= 'A' && c <= 'Z') {
                c = c + 32; 
            }
            palabra += c;
        }
        pos_en_texto++;
    }

    lock_guard<mutex> lock(mtx);
    for (size_t i = 0; i < conteo_local.size(); i++) {
        conteo_global[i] += conteo_local[i];
    }
}

int main() {
    // Abrir archivos globales una vez
    for (const auto& p : dic) {
        string filename = "posiciones_" + p.first + ".txt";
        archivos_globales[p.second] = fopen(filename.c_str(), "w");
        if (!archivos_globales[p.second]) {
            cerr << "Error abriendo " << filename << endl;
            return 1;
        }
        setvbuf(archivos_globales[p.second], nullptr, _IOFBF, 64 * 1024); 
    }

    FILE* file = fopen("wikipedia.txt", "rb");
    if (!file) {
        cerr << "No se pudo abrir el archivo test.txt\n";
        return 1;
    }
    setvbuf(file, nullptr, _IOFBF, 128 * 1024 * 1024); // Buffer grande para lectura

    vector<char> buffer(batch_size);
    string bloque;
    long long offset_global = 0;

    while (true) {
        bloque.clear();
        size_t leidos = fread(buffer.data(), 1, buffer.size(), file);
        if (leidos == 0) break;

        bloque.assign(buffer.data(), leidos);
        if (!feof(file) && bloque.back() != ' ') {
            int c;
            while ((c = fgetc(file)) != EOF) {
                bloque.push_back((char)c);
                if (c == ' ') break;
            }
        }

        int parte_size = bloque.size() / num_threads;
        int ini = 0;
        vector<thread> hilos;
        vector<vector<vector<long long>>> all_local_pos(num_threads, vector<vector<long long>>(tam_dic));

        for (int i = 0; i < num_threads; i++) {
            int fin = (i == num_threads - 1) ? bloque.size() : ini + parte_size;
            if (fin < (int)bloque.size() && bloque[fin] != ' ') {
                while (fin < (int)bloque.size() && bloque[fin] != ' ') fin++;
            }
            string subtexto = bloque.substr(ini, fin - ini);
            long long offset_subtexto = offset_global + ini;
            hilos.emplace_back(cont_palabras, subtexto, offset_subtexto, ref(all_local_pos[i]));
            ini = fin;
        }

        for (auto& h : hilos) h.join();

        // Escribir posiciones a archivos globales
        for (int w = 0; w < tam_dic; ++w) {
            FILE* out = archivos_globales[w];
            for (int t = 0; t < num_threads; ++t) {
                for (long long pos : all_local_pos[t][w]) {
                    fprintf(out, "%lld\n", pos);
                }
            }
        }

        offset_global += bloque.size();
    }

    // Cerrar archivos
    fclose(file);
    for (auto f : archivos_globales) fclose(f);

    cout << "Resultados finales:" << endl;
    for (const auto& p : dic) {
        cout << p.first << ": " << conteo_global[p.second] << endl;
    }
    return 0;
}

