#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <queue>
#include <unordered_map>
#include <vector>

using namespace std;

struct BufferEntry {
  int frame_id;
  int page_id;
  int operacion; // 0 = lectura, 1 = escritura
  int dirty;     // 0 = no, 1 = si
  int pin_count; // cantidad de pines
  bool pin_fijo; // true = no se puede evictar
};

class Buffer {
private:
  int max_frames;
  unordered_map<int, BufferEntry> buffer_map; 
  unordered_map<int, int> page_to_frame;      
  vector<int> frame_to_page;                  
  vector<queue<int>> operaciones;
  vector<bool> use_bit; 
  int manecilla = 0;

  bool eliminarPagina() {
    int intentos = 0;
    bool dio_vuelta = false;

    while (true) {
        int frame = manecilla;
        int candidate_page = frame_to_page[frame];

        if (candidate_page != -1) {
            BufferEntry &entry = buffer_map[candidate_page];

            if (!entry.pin_fijo) {
                if (entry.pin_count == 0) {
                    if (use_bit[frame]) {
                        use_bit[frame] = false; // Segunda oportunidad
                    } else {

                      /*
                        // Evictar
                        if (entry.dirty)
                            cout << "Escribiendo pagina " << candidate_page << " al disco (dirty).\n";
                        else
                            cout << "Descartando pagina limpia " << candidate_page << ".\n";
                      */

                        buffer_map.erase(candidate_page);
                        page_to_frame.erase(candidate_page);
                        frame_to_page[frame] = -1;
                        operaciones[frame] = queue<int>();
                        use_bit[frame] = false;

                        manecilla = (manecilla + 1) % max_frames;
                        return true;
                    }
                } else {
                    // Despin automáticas de operaciones pendientes
                    while (!operaciones[frame].empty()) {
                        int last_op = operaciones[frame].front();
                        operaciones[frame].pop();
                        entry.pin_count--;

                        if (last_op == 1) {
                            //cout << "Guardando automaticamente pagina " << entry.page_id << ".\n";
                            entry.dirty = false;
                        }
                    }
                }
            }
        }

        manecilla = (manecilla + 1) % max_frames;
        intentos++;

        if (intentos >= max_frames && !dio_vuelta) {
            // Primera vuelta completa: no encontró página evictable
            dio_vuelta = true;

            //cout << "Todas las paginas tienen pin fijo. Forzando despine automatico.\n";

            // Forzar despine de la página donde está el manecilla
            int frame_forzado = manecilla % max_frames; // para asegurar dentro del rango
            int page_id = frame_to_page[frame_forzado];
            if (page_id != -1) {
                buffer_map[page_id].pin_fijo = false;
                //cout << "Despined automatico de la pagina " << page_id << ".\n";
            }

            // Reintentar ciclo completo desde el inicio
            intentos = 0;
        }

        if (intentos >= max_frames && dio_vuelta) {
            // Segunda vuelta completa sin éxito: algo raro pasó
            //cout << "Error: No se pudo liberar espacio en el buffer incluso tras despine forzado.\n";
            return false;
        }
    }
}



public:
  Buffer(int size) : max_frames(size) {
    frame_to_page.resize(size, -1);
    operaciones.resize(size);
    use_bit.resize(size, false);
  }

  bool loadPage(int page_id, int operacion, bool pin_fijo = false) {
    if (buffer_map.count(page_id)) {
        BufferEntry &entry = buffer_map[page_id];
        entry.pin_count++;
        entry.operacion = operacion;
        entry.dirty = operacion;
        operaciones[entry.frame_id].push(operacion);
        use_bit[entry.frame_id] = true;
        //cout << "HIT: Pagina " << page_id << " ya esta cargada. Pin count ahora = " << entry.pin_count << "\n";
        return true;
    }

    if ((int)buffer_map.size() >= max_frames) {
        bool evicted = eliminarPagina();

        if (!evicted) {
            //cout << "ERROR: No se pudo cargar la pagina " << page_id << " porque todas las paginas tienen pin fijo.\n";
            return false;
        }
    }

    int assigned_frame = -1;
    for (int i = 0; i < max_frames; ++i) {
        if (frame_to_page[i] == -1) {
            assigned_frame = i;
            break;
        }
    }

    if (assigned_frame == -1) {
        //cout << "Error: no se encontró frame libre tras evicción.\n";
        return false;
    }

    bool es_dirty = (operacion == 1);
    BufferEntry entry = {assigned_frame, page_id, operacion, es_dirty ? 1 : 0, 1, pin_fijo};

    buffer_map[page_id] = entry;
    page_to_frame[page_id] = assigned_frame;
    frame_to_page[assigned_frame] = page_id;
    operaciones[assigned_frame].push(operacion);
    use_bit[assigned_frame] = true;

    //cout << "Cargada pagina " << page_id << " en frame " << assigned_frame << ".\n";
    return true;
}


  void setPinFijo(int page_id, bool value) {
    if (buffer_map.count(page_id)) {
      buffer_map[page_id].pin_fijo = value;
      //cout << "Pagina " << page_id << " ahora tiene pin fijo = " << (value ? "Si" : "No") << ".\n";
    }
  }

  void Mostrar() const {
    cout << "\n--- Estado del Clock Buffer ---\n";
    for (int i = 0; i < max_frames; ++i) {
        int pid = frame_to_page[i];
        if (pid != -1) {
            const BufferEntry &e = buffer_map.at(pid);

            cout << "FRAMEID: " << e.frame_id
                 << " | PAGEID: B" << e.page_id
                 << " | MODO: " << (e.operacion == 0 ? "R" : "W")
                 << " | DIRTY: " << (e.dirty ? "si" : "no")
                 << " | PINCOUNT: " << e.pin_count
                 << " | REFBIT: " << (use_bit[i] ? "1" : "0")
                 << " | PINNED: " << (e.pin_fijo ? "si" : "no")
                 << "\n";
        } else {
            cout << "FRAMEID: " << i << " | <vacio>\n";
        }
    }
}

  void mostrarContenidoPagina(int page_id, const vector<pair<string, vector<string>>>& bloques) {
    string nombre_bloque = "B" + to_string(page_id);

    for (const auto& bloque : bloques) {
        if (bloque.first == nombre_bloque) {
            cout << "Contenido de " << nombre_bloque << ":\n";
            for (const string& ruta : bloque.second) {
                ifstream in(ruta);
                if (in.is_open()) {
                    string linea;
                    while (getline(in, linea)) {
                        cout << linea << '\n';
                    }
                    in.close();
                } else {
                    cout << "No se pudo abrir " << ruta << '\n';
                }
            }
            return;
        }
    }

    cout << "Bloque " << nombre_bloque << " no encontrado.\n";
}



  void MostrarOP() {
    cout << "\nOperaciones pendientes\n";
    for (size_t i = 0; i < operaciones.size(); ++i) {
      cout << "  Frame " << i << ": ";
      queue<int> copia = operaciones[i];
      while (!copia.empty()) {
        cout << ((copia.front() == 0) ? "L" : "W") << " ";
        copia.pop();
      }
      cout << "\n";
    }
  }
};

#endif
