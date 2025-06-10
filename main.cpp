#include <iostream>
#include <fstream>
#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdlib>


using namespace std;

const int platos = 5;
const int superficies = 2;
const int pistas = 10;
const int sectores = 10;
const int TAM_CH = 300;
const int MAX_C  = 100;
const int TAM_COOR = 4;
const int MAX_COLS = 50;
const int MAX_LINE_LEN = 2048;

const int TOTAL_SECTORES = platos * superficies * pistas * sectores;

class Sectores {
public:
    Sectores(const char* ruta, int sec_num, bool es_esquema) {
        char ruta_Archivo[TAM_CH];

        sprintf(ruta_Archivo, "%s\\Sector%d.txt", ruta, sec_num);

        ofstream archivo(ruta_Archivo);
        archivo.close();
    }
};

class Pista {
public:
    Pista(const char* baseRuta, int pl, int sup, int pt, int& contador_sectores) {
        char ruta[TAM_CH];
        sprintf(ruta, "%s\\Pista%d", baseRuta, pt);
        CreateDirectoryA(ruta, NULL);

        for (int i = 0; i < sectores; i++) {
            bool es_esquema = (pl == 0 && sup == 0 && pt == 0 && i == 0);
            Sectores s(ruta, i, es_esquema);
        }
    }
};

class Superficies {
public:
    Superficies(const char* baseRuta, int pl, int sup) {
        char ruta[TAM_CH];
        sprintf(ruta, "%s\\Superficie%d", baseRuta, sup);
        CreateDirectoryA(ruta, NULL);

        int contador_sectores = 0;
        for (int i = 0; i < pistas; i++) {
            Pista p(ruta, pl, sup, i, contador_sectores);
        }
    }
};

class Plato {
public:
    Plato(const char* baseRuta, int pl) {
        char ruta[TAM_CH];
        sprintf(ruta, "%s\\Plato%d", baseRuta, pl);
        CreateDirectoryA(ruta, NULL);

        for (int i = 0; i < superficies; i++) {
            Superficies s(ruta, pl, i);
        }
    }
};

class Disco {
public:
    Disco(const char* nombre) {
        CreateDirectoryA(nombre, NULL);

        for (int i = 0; i < platos; i++) {
            Plato plato(nombre, i);
        }
    }

    void coordenadas(int index, int& pl, int& sup, int& pt, int& sec) {
        int en_plato = superficies * pistas * sectores;
        int en_superficie = pistas * sectores;
        int en_pistas = sectores;

        pl = index / en_plato;
        sup = (index % en_plato) / en_superficie;
        pt = (index % en_superficie) / en_pistas;
        sec = index % sectores;
    }


    string rutaSector(int index) {
        int pl, sup, pt, sec;
        coordenadas(index, pl, sup, pt, sec);

        char ruta[TAM_CH];
        sprintf(ruta, "Disco\\Plato%d\\Superficie%d\\Pista%d\\Sector%d.txt", pl, sup, pt, sec);
        return string(ruta);
    }
};

void header(const char rutas_bloque[][TAM_CH], int n_b, int bloque_id, int tam_bloque){
    if(n_b <= 0){
        return;
    }

    stringstream header;
    header << "ID:" << bloque_id + 1 << "#R:0" << "#Lib:" << tam_bloque << '\n';

    ofstream sector0(rutas_bloque[0]);
    if(!sector0.is_open()){
        cerr << "No se coloco el header: " << rutas_bloque[0] << endl;
        return;
    }

    sector0 << header.str();
    sector0.close();
}

void actualizar_header(const char* ruta_sector0, int id_bloque, int registros, int espacio, int tam_fijo) {
    fstream archivo(ruta_sector0, ios::in | ios::out);
    if (!archivo.is_open()) {
        cerr << "Error al abrir el header para actualizar: " << ruta_sector0 << endl;
        return;
    }

    stringstream header_nuevo;
    header_nuevo << "ID:" << id_bloque + 1 << "#R:" << registros << "#Lib:" << espacio << "#Tam_fijo:" << tam_fijo <<'\n';

    archivo.seekp(0);
    archivo << header_nuevo.str();
    archivo.close();
}


void Creacion_bloques(Disco& d, int tam_sector, int n_b) {
    int total_superficies = platos * superficies;
    int sectores_por_superficie = pistas * sectores;
    char rutas_de_bloque[MAX_C][TAM_CH];

    vector<int> cursor_por_superficie(total_superficies, 0);
    vector<string> rutas_bloque_B0;

    int bloque_id = 0;
    bool quedan_sectores = true;
    bool bloque_B0_guardado = false;

    vector<string> lineas_bloques;

    while (quedan_sectores) {
        quedan_sectores = false;

        for (int s = 0; s < total_superficies; ++s) {
            if (cursor_por_superficie[s] >= sectores_por_superficie)
                continue;

            quedan_sectores = true;

            std::stringstream linea;
            linea << "B" << bloque_id << ":";

            int pl = s / superficies;
            int sp = s % superficies;

            int agregados = 0;

            for (int pt = 0; pt < pistas && agregados < n_b; ++pt) {
                for (int sec = 0; sec < sectores && agregados < n_b; ++sec) {
                    int flat_index = pl * superficies * pistas * sectores +
                                     sp * pistas * sectores +
                                     pt * sectores +
                                     sec;

                    int sector_consumido = pt * sectores + sec;

                    if (sector_consumido >= cursor_por_superficie[s]) {
                        int p, su, pi, se;
                        d.coordenadas(flat_index, p, su, pi, se);
                        linea << p << su << pi << se << '#';

                        // guardar ruta solo si se va a usar
                        string ruta_sector = d.rutaSector(flat_index);
                        strcpy(rutas_de_bloque[agregados], ruta_sector.c_str());

                        if (!bloque_B0_guardado)
                            rutas_bloque_B0.push_back(ruta_sector);

                        agregados++;
                        cursor_por_superficie[s]++;
                    }
                }
            }

            // Guardar la línea de coordenadas
            lineas_bloques.push_back(linea.str());

            // Escribir header del bloque recién creado
            header(rutas_de_bloque, n_b, bloque_id, tam_sector * n_b);

            bloque_id++;

            if (!bloque_B0_guardado && rutas_bloque_B0.size() == (size_t)n_b)
                bloque_B0_guardado = true;
        }
    }

    // Agrupar líneas de instrucciones (bloque B0)
    vector<string> sectores_texto;
    std::stringstream sector_actual;
    int tam_actual = 0;

    for (const string& linea : lineas_bloques) {
        if ((int)linea.size() + tam_actual > tam_sector) {
            sectores_texto.push_back(sector_actual.str());
            sector_actual.str("");
            sector_actual.clear();
            tam_actual = 0;
        }

        sector_actual << linea << "\n";
        tam_actual += linea.size() + 1;
    }

    if (tam_actual > 0)
        sectores_texto.push_back(sector_actual.str());

    // Validación de capacidad para B0
    if ((int)sectores_texto.size() > n_b) {
        cerr << "ERROR: Las líneas no caben completas en el bloque B0 con " 
             << n_b << " sectores de " << tam_sector << " bytes.\n";
        return;
    }

    // Escribir las instrucciones en el bloque B0
    for (size_t i = 0; i < sectores_texto.size(); ++i) {
        ofstream archivo(rutas_bloque_B0[i]);
        archivo << sectores_texto[i];
        archivo.close();
    }

    // Rellenar sectores vacíos en B0
    for (size_t i = sectores_texto.size(); i < (size_t)n_b; ++i) {
        ofstream archivo(rutas_bloque_B0[i]);
        archivo.close();
    }
}


void limpiar_campo(char* campo){
    while (*campo == ' ' || *campo == '\t' || *campo == '\r' || *campo == '\n') {
        ++campo;
    }
    if (*campo == '"') {
        ++campo;
    }

    char limpio[MAX_LINE_LEN];
    strcpy(limpio, campo);

    int len = strlen(limpio);
    while (len > 0 && (limpio[len-1] == ' ' || limpio[len-1] == '\t' || limpio[len-1] == '\r' || limpio[len-1] == '\n')) {
        limpio[len-1] = '\0';
        --len;
    }
    if (len > 0 && limpio[len-1] == '"') {
        limpio[len-1] = '\0';
    }

    strcpy(campo, limpio);
}

void obtener_esquema(const char* archivo_csv, char nombres_columnas[MAX_COLS][MAX_LINE_LEN], char tipos_columnas[MAX_COLS][10], int& num_cols, char esquema[]) {
    ifstream archivo(archivo_csv);
    if (!archivo.is_open()) {
        cout << "Error al abrir el archivo.\n";
        return;
    }

    char linea[MAX_LINE_LEN];

    archivo.getline(linea, MAX_LINE_LEN);
    char* token = strtok(linea, ",");
    num_cols = 0;
    while(token != NULL && num_cols < MAX_COLS){
        limpiar_campo(token);
        strcpy(nombres_columnas[num_cols], token);
        token = strtok(NULL, ",");
        num_cols++;
    }

    //seg linea
    if(archivo.getline(linea, MAX_LINE_LEN)){
        token = strtok(linea, ",");
        int col_index = 0;
        while (token != NULL && col_index < num_cols) {
            limpiar_campo(token);

            //tipo
            char* endptr;
            strtol(token, &endptr, 10);
            if (*endptr == '\0') {
                strcpy(tipos_columnas[col_index], "int");
            } else {
                strtof(token, &endptr);
                if (*endptr == '\0') {
                    strcpy(tipos_columnas[col_index], "float");
                } else {
                    strcpy(tipos_columnas[col_index], "string");
                }
            }

            token = strtok(NULL, ",");
            ++col_index;
        }           
    }
    for (int i = 0; i < num_cols; ++i) {
        strcat(esquema, nombres_columnas[i]);
        strcat(esquema, "#");
        strcat(esquema, tipos_columnas[i]);
        if (i < num_cols - 1) strcat(esquema, "#");
    }

    archivo.close();
}

int hallar_tam_fijo(const char* archivo_csv, char tipos_columnas[MAX_COLS][10], int num_cols, int max_int, int max_float, int max_char) {
    ifstream archivo(archivo_csv);
    if (!archivo.is_open()) {
        cout << "Error al abrir el archivo.\n";
        return -1;
    }

    char linea[MAX_LINE_LEN];
    archivo.getline(linea, MAX_LINE_LEN);

    int max_len_string[MAX_COLS];
    for (int i = 0; i < num_cols; ++i) {
        max_len_string[i] = 0;
    }

    // Leer cada línea
    while (archivo.getline(linea, MAX_LINE_LEN)) {
        char* token = strtok(linea, ",");
        int col_index = 0;
        while (token != NULL && col_index < num_cols) {
            limpiar_campo(token);
            if (strcmp(tipos_columnas[col_index], "string") == 0) {
                int len = strlen(token);
                if (len > max_len_string[col_index]) {
                    max_len_string[col_index] = len;
                }
            }
            token = strtok(NULL, ",");
            ++col_index;
        }
    }

    archivo.close();

    // Sumar tam_fijo
    int tam_fijo = 0;
    for (int i = 0; i < num_cols; ++i) {
        if (strcmp(tipos_columnas[i], "int") == 0) {
            tam_fijo += max_int;
        } else if (strcmp(tipos_columnas[i], "float") == 0) {
            tam_fijo += max_float;
        } else if (strcmp(tipos_columnas[i], "string") == 0) {
            tam_fijo += max_char;
        }
    }

    return tam_fijo;
}


void guardar_csv(Disco& d, const char* ruta, int tam_sector, int tam_bloque, int n_b, int tam_fijo) {


    int pl = 0, sup = 0, pt = 0, sec = 0;
    char ruta_bloques[TAM_CH];
    sprintf(ruta_bloques, "Disco\\Plato%d\\Superficie%d\\Pista%d\\Sector%d.txt", pl, sup, pt, sec);

    ifstream csv(ruta);
    if (!csv.is_open()) {
        cerr << "Error al abrir el csv: " << ruta << endl;
        return;
    }

    ofstream temp("temp.txt");
    if (!temp.is_open()) {
        cerr << "No se pudo crear el archivo temporal." << endl;
        return;
    }

    // Saltar encabezado
    char c;
    bool dentroComillas = false;

    // Saltar primera línea (encabezado)
    while (csv.get(c) && c != '\n') {}

    // Ahora procesar el resto del archivo
    while (csv.get(c)) {
        if (c == '"') {
            dentroComillas = !dentroComillas;
            csv.get(c);  // Leer siguiente caracter (puede ser el caracter dentro de las comillas o después de cerrar comillas)
        }

        if (dentroComillas) {
            temp << c;
        } else {
            if (c == ',') {
                temp << '#';
            } else if (c == '\n') {
                temp << '\n';
            } else {
                temp << c;
            }
        }
    }

    temp << '\n';  // Asegurarse de terminar con salto de línea
    temp.close();
}


void importar_disco(Disco& d, const char* archivo, int tam_sector, int tam_bloque, int n_b, int tam_fijo,
                    const char* esquema, int bloques_usados[MAX_C], int total_bloques_usados,
                    int tam_columna[MAX_COLS], int num_cols, char tipos_columnas[MAX_COLS][10]) {
    int pl = 0, sup = 0, pt = 0, sec = 0;
    char ruta[TAM_CH];
    sprintf(ruta, "Disco\\Plato%d\\Superficie%d\\Pista%d\\Sector%d.txt", pl, sup, pt, sec);
    ifstream bloque_d(ruta);
    if (!bloque_d.is_open()) {
        cerr << "Error al abrir el archivo: " << ruta << endl;
        return;
    }

    string dummy;
    getline(bloque_d, dummy);
    string linea;
    getline(bloque_d, linea);

    int pos = linea.find(':');
    if (pos == string::npos) {
        cerr << "Las coordenadas no se guardaron correctamente" << endl;
        return;
    }

    linea = linea.substr(pos + 1);
    int coordenadas[n_b][TAM_COOR];
    int i = 0, inicio = 0;

    while (inicio < linea.length() && i < n_b) {
        int fin = linea.find('#', inicio);
        if (fin == string::npos) break;

        string codigo = linea.substr(inicio, fin - inicio);
        if (codigo.length() == 4) {
            coordenadas[i][0] = codigo[0] - '0';
            coordenadas[i][1] = codigo[1] - '0';
            coordenadas[i][2] = codigo[2] - '0';
            coordenadas[i][3] = codigo[3] - '0';
            i++;
        }

        inicio = fin + 1;
    }

    ifstream reg("temp.txt");
    if (!reg.is_open()) {
        cerr << "Error al abrir el archivo: temp.txt" << endl;
        return;
    }

    int current_sector_idx = 0;
    int bloque_actual = 0;
    int contador_registros = 0;
    int espacio_restante = tam_bloque;

    char ruta_primer_sector[TAM_CH];
    sprintf(ruta_primer_sector, "Disco\\Plato%d\\Superficie%d\\Pista%d\\Sector%d.txt",
            coordenadas[0][0], coordenadas[0][1], coordenadas[0][2], coordenadas[0][3]);

    ofstream sect_d;
    int contador_bytes_sector = 0;

    // Abrir primer sector y poner el esquema
    pl = coordenadas[0][0];
    sup = coordenadas[0][1];
    pt = coordenadas[0][2];
    sec = coordenadas[0][3];

    char ruta_disco[TAM_CH];
    sprintf(ruta_disco, "Disco\\Plato%d\\Superficie%d\\Pista%d\\Sector%d.txt", pl, sup, pt, sec);

    string header_line;
    {
        ifstream entrada(ruta_disco);
        if (!entrada.is_open()) {
            cerr << "Error al abrir sector para lectura: " << ruta_disco << endl;
            return;
        }
        getline(entrada, header_line);
        entrada.close();

        ofstream salida(ruta_disco);
        if (!salida.is_open()) {
            cerr << "Error al abrir sector para escritura: " << ruta_disco << endl;
            return;
        }
        salida << header_line << '\n';
        salida << esquema << '\n';
        salida.close();

        contador_bytes_sector = strlen(esquema) + 1;
    }

    sect_d.open(ruta_disco, ios::app);
    if (!sect_d.is_open()) {
        cerr << "Error al reabrir para registros: " << ruta_disco << endl;
        return;
    }

    string registro;
    while (getline(reg, registro)) {
        char buffer[tam_fijo];
        memset(buffer, ' ', tam_fijo);

        stringstream ss(registro);
        string campo;
        int offset = 0;

        for (int i = 0; i < num_cols && getline(ss, campo, '#'); ++i) {
            int len = campo.length();
            int tam_col = tam_columna[i];
            int copiar = min(len, tam_col);
            memcpy(buffer + offset, campo.c_str(), copiar);
            offset += tam_col;
        }

        int tam_registro = tam_fijo + 1;

        if (contador_bytes_sector + tam_registro > tam_sector) {
            sect_d.close();
            current_sector_idx++;

            if (current_sector_idx >= n_b) {
                actualizar_header(ruta_primer_sector, bloque_actual, contador_registros, espacio_restante, tam_fijo);

                string lineaBloque;
                while (getline(bloque_d, lineaBloque)) {
                    if (lineaBloque[0] == 'B') break;
                }

                pos = lineaBloque.find(':');
                if (pos == string::npos) {
                    cerr << "Error: formato de bloque inválido." << endl;
                    return;
                }

                lineaBloque = lineaBloque.substr(pos + 1);
                i = 0;
                inicio = 0;
                while (inicio < lineaBloque.length() && i < n_b) {
                    int fin = lineaBloque.find('#', inicio);
                    if (fin == string::npos) break;

                    string codigo = lineaBloque.substr(inicio, fin - inicio);
                    if (codigo.length() == 4) {
                        coordenadas[i][0] = codigo[0] - '0';
                        coordenadas[i][1] = codigo[1] - '0';
                        coordenadas[i][2] = codigo[2] - '0';
                        coordenadas[i][3] = codigo[3] - '0';
                        i++;
                    }
                    inicio = fin + 1;
                }

                bloque_actual++;
                current_sector_idx = 0;
                contador_registros = 0;
                espacio_restante = tam_bloque;

                sprintf(ruta_primer_sector, "Disco\\Plato%d\\Superficie%d\\Pista%d\\Sector%d.txt",
                        coordenadas[0][0], coordenadas[0][1], coordenadas[0][2], coordenadas[0][3]);
            }

            pl = coordenadas[current_sector_idx][0];
            sup = coordenadas[current_sector_idx][1];
            pt = coordenadas[current_sector_idx][2];
            sec = coordenadas[current_sector_idx][3];

            sprintf(ruta_disco, "Disco\\Plato%d\\Superficie%d\\Pista%d\\Sector%d.txt", pl, sup, pt, sec);
            sect_d.open(ruta_disco, ios::app);
            if (!sect_d.is_open()) {
                cerr << "Error al abrir el sector: " << ruta_disco << endl;
                return;
            }

            contador_bytes_sector = 0;

            if (current_sector_idx == 0) {
                bloques_usados[total_bloques_usados++] = bloque_actual;
                sect_d << esquema << '\n';
                contador_bytes_sector += strlen(esquema) + 1;
            }
        }

        for (int j = 0; j < tam_fijo; j++) {
            sect_d << buffer[j];
        }
        sect_d << '\n';

        contador_bytes_sector += tam_registro;
        contador_registros++;
        espacio_restante -= tam_registro;
    }

    sect_d.close();
    actualizar_header(ruta_primer_sector, bloque_actual, contador_registros, espacio_restante, tam_fijo);
    reg.close();

    cout << "Bloques utilizados:\n";
    for (int i = 0; i < total_bloques_usados; i++) {
        cout << "B" << bloques_usados[i] << " ";
    }
    cout << endl;
}




int main(){
    int tam_bloque, tam_sector, n_b;
    string nombreArchivo;
    int longitudes[MAX_C];
    char nombres_columnas[MAX_COLS][MAX_LINE_LEN];
    char tipos_columnas[MAX_COLS][10];
    int num_cols = 0;
    char esquema[MAX_LINE_LEN];
    int tam_fijo = 0;
    int opcion = 0;
    int bloques_usados[MAX_C]; // arreglo para almacenar los bloques usados
    int total_bloques_usados = 0;
    int max_int = 0;
    int max_float = 0;
    int max_char = 0;
    int capacidad_total;
    Disco* d = nullptr;

    //Menu    
    do{
        cout << "% MEGATRON3000\n Welcome to MEGATRON 3000!\n";
        cout << "1.- Crear disco" << endl;
        cout << "2.- Importar csv a disco" << endl;
        //cout << "3.- Buscar un registro" << endl;
        //cout << "4.- Agregar un registro" << endl;
        //cout << "5.- Eliminar un registro" << endl;
        cout << "3.- Salir" << endl;
        cout << "Ingrese su opcion: ";
        cin >> opcion;

        switch(opcion){
            case 1:{
                cout << "Numero de bytes del sector: ";
                cin >> tam_sector;
                cout << "Numero de sectores por bloque: ";
                cin >> n_b;
                tam_bloque = tam_sector * n_b;
                if (d != nullptr) delete d;  // Limpia si ya existía uno antes
                d = new Disco("Disco");
                Creacion_bloques(*d, tam_sector, n_b);
                cout << "Disco creado correctamente" << endl;
                capacidad_total = TOTAL_SECTORES * tam_sector;
                cout << "Capacidad total: " << capacidad_total << " bytes" << endl;
                break;
            }

            case 2: {
                if (d == nullptr) {
                    cout << "Primero debe crear el disco (opcion 1).\n";
                    break;
                }

                cout << "Nombre del archivo CSV: ";
                cin >> nombreArchivo;

                cout << "Tamano de los int: ";
                cin >> max_int;
                cout << "Tamano de los float: ";
                cin >> max_float;
                cout << "Tamano de los string: ";
                cin >> max_char;

                const char* archivo = nombreArchivo.c_str();
                memset(esquema, 0, sizeof(esquema));
                obtener_esquema(archivo, nombres_columnas, tipos_columnas, num_cols, esquema);

                int tam_columna[MAX_COLS];
                tam_fijo = 0;

                for (int i = 0; i < num_cols; ++i) {
                    if (strcmp(tipos_columnas[i], "int") == 0) {
                        tam_columna[i] = max_int;
                    } else if (strcmp(tipos_columnas[i], "float") == 0) {
                        tam_columna[i] = max_float;
                    } else if (strcmp(tipos_columnas[i], "string") == 0) {
                        tam_columna[i] = max_char;
                    }
                    tam_fijo += tam_columna[i];
                }

                guardar_csv(*d, archivo, tam_sector, tam_bloque, n_b, tam_fijo);

                importar_disco(*d, "temp.txt", tam_sector, tam_bloque, n_b, tam_fijo, esquema,
                            bloques_usados, total_bloques_usados, tam_columna, num_cols, tipos_columnas);

                cout << "CSV importado correctamente.\n";
                break;
            }
        }

    }while (opcion != 3);

    cout << "Saliendo del programa...";
    delete d;
    return 0;
}
