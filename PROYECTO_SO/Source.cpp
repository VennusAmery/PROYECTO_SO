#include <iostream>
#include <queue>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <set>
#include <stdexcept>

using namespace std;

class Proceso {
public:
    int pid;
    int ppid;
    string pc;
    int registros;
    int tamano;
    int hilos;
    int quantum;
    int iteracion;
    string estado;
    int tiempo_ejecucion;

    Proceso(int _pid, int _ppid, string _pc, int _registros, int _tamano, int _hilos, int _quantum, int _iteracion) {
        pid = _pid;
        ppid = _ppid;
        pc = _pc;
        registros = _registros;
        tamano = _tamano;
        hilos = _hilos;
        quantum = _quantum;
        iteracion = _iteracion;
        estado = "listo";
        tiempo_ejecucion = 0;
    }

    void ejecutar(int quantum, int hilos_procesador) {
        cout << "Ejecutando Proceso " << pid << " en estado " << estado << endl;
        estado = "ejecucion";

        if (hilos_procesador <= 0) {
            cerr << "Error: El numero de hilos de procesador debe ser mayor que 0." << endl;
            exit(1);
        }
        int tiempo_ejecucion_real = quantum / hilos_procesador;  // Simula mayor velocidad con más hilos
        this_thread::sleep_for(chrono::seconds(tiempo_ejecucion_real));

        iteracion--;
        tiempo_ejecucion += tiempo_ejecucion_real;

        if (iteracion <= 0) {
            estado = "terminado";
            cout << "Proceso " << pid << " ha terminado" << endl;
        }
        else {
            estado = "listo";
            cout << "Proceso " << pid << " ha completado " << tiempo_ejecucion_real
                << " segundos de ejecucion, le quedan " << iteracion << " iteraciones" << endl;
        }
    }
};

class MultilevelFeedbackQueueScheduler {
public:
    queue<Proceso*> cola_A;
    queue<Proceso*> cola_B;
    queue<Proceso*> cola_C;
    int tiempo_total;
    int ultimo_tiempo_promocion;

    MultilevelFeedbackQueueScheduler(int hilos_totales) {
        tiempo_total = 0;
        ultimo_tiempo_promocion = 0;
    }

    void agregar_proceso(Proceso* proceso) {
        cout << "Agregando Proceso " << proceso->pid << " a la cola de alta prioridad" << endl;
        cola_A.push(proceso);
    }

    void promocionar_procesos() {
        cout << "Promoviendo todos los procesos a la cola de alta prioridad para evitar Starvation" << endl;
        while (!cola_B.empty()) {
            cola_A.push(cola_B.front());
            cola_B.pop();
        }
        while (!cola_C.empty()) {
            cola_A.push(cola_C.front());
            cola_C.pop();
        }
        ultimo_tiempo_promocion = tiempo_total;
    }

    void ejecutar_procesos(int hilos_procesador) {
        while (!cola_A.empty() || !cola_B.empty() || !cola_C.empty()) {
            if (tiempo_total - ultimo_tiempo_promocion >= 60) {
                promocionar_procesos();
            }

            if (!cola_A.empty()) {
                ejecutar_proceso_de_cola(cola_A, hilos_procesador);
            }
            else if (!cola_B.empty()) {
                ejecutar_proceso_de_cola(cola_B, hilos_procesador);
            }
            else if (!cola_C.empty()) {
                ejecutar_proceso_de_cola(cola_C, hilos_procesador);
            }
        }
    }

private:
    void ejecutar_proceso_de_cola(queue<Proceso*>& cola, int hilos_procesador) {
        Proceso* proceso = cola.front();
        cola.pop();
        proceso->ejecutar(proceso->quantum, hilos_procesador);
        tiempo_total += proceso->quantum;

        if (proceso->estado != "terminado") {
            cola.push(proceso);
        }
    }
};


// Función para cargar procesos que no sean 0
void cargar_procesos_desde_archivo(string archivo, MultilevelFeedbackQueueScheduler& scheduler, int& procesadores, int& hilos_totales) {
    ifstream infile(archivo);
    string line;
    int linea_actual = 0;
    set<int> pids;  // Conjunto para almacenar los PIDs únicos

    while (getline(infile, line)) {
        linea_actual++;
        istringstream ss(line);
        string item;

        // Leer las dos primeras líneas para capturar los procesadores e hilos
        if (linea_actual == 1) {
            ss >> item >> procesadores;  // Leer el número de procesadores
            if (procesadores <= 0) {
                cout << "Error: El numero de procesadores no puede ser 0 o menor. Por favor, ingrese un numero valido." << endl;
                return;
            }
            continue;
        }
        if (linea_actual == 2) {
            ss >> item >> hilos_totales;  // Leer el número de hilos totales
            if (hilos_totales <= 0) {
                cout << "Error: El numero de hilos no puede ser 0 o menor. Por favor, ingrese un numero valido." << endl;
                return;
            }
            continue;
        }

        // A partir de la tercera línea, procesamos los procesos
        vector<string> data;

        // Parsear los datos del proceso desde la línea usando '|' como delimitador
        while (getline(ss, item, '|')) {
            data.push_back(item);
        }

        // Verificar que se hayan leído todos los campos necesarios
        if (data.size() != 8) {
            cout << "Error: Numero incorrecto de campos en la linea: " << line << endl;
            continue;
        }

        try {
            // Validar los valores numéricos
            int pid = stoi(data[0]);
            int ppid = stoi(data[1]);
            string pc = data[2];
            int registros = stoi(data[3]);
            int tamano = stoi(data[4]);
            int hilos = stoi(data[5]);
            int quantum = stoi(data[6]);
            int iteracion = stoi(data[7]);

            // Validar que ningún valor numérico sea negativo o cero
            if (pid <= 0 || registros <= 0 || tamano <= 0 || hilos <= 0 || quantum <= 0 || iteracion <= 0) {
                cout << "Error: Proceso con valores negativos o cero en la linea: " << line << endl;
                continue;
            }

            // Verificar si el PID ya ha sido agregado
            if (pids.find(pid) != pids.end()) {
                cout << "Proceso con PID " << pid << " ya ha sido cargado, omitiendo duplicado en la linea: " << line << endl;
                continue;  // Ignorar el proceso duplicado
            }

            // Si el PID no está duplicado, agregarlo al conjunto
            pids.insert(pid);

            // Crear el proceso y agregarlo al scheduler
            Proceso* proceso = new Proceso(pid, ppid, pc, registros, tamano, hilos, quantum, iteracion);
            scheduler.agregar_proceso(proceso);
        }
        catch (const invalid_argument& e) {
            cout << "Error: Caracteres no numericos en un campo numerico. Linea: " << line << endl;
        }
        catch (const out_of_range& e) {
            cout << "Error: Valor fuera de rango en la linea: " << line << ". Error: " << e.what() << endl;
        }
    }
    infile.close();
}

// Función para validar si tiene decimales
void contiene_decimales(const string& nombreArchivo) {
    ifstream archivo(nombreArchivo);
    string linea;

    // Verificamos si el archivo se pudo abrir correctamente
    if (!archivo.is_open()) {
        cout << "No se pudo abrir el archivo: " << nombreArchivo << endl;
        return;
    }

    // Leemos línea por línea
    while (getline(archivo, linea)) {
        // Recorremos cada carácter 
        bool tiene_decimales = false;
        for (char c : linea) {
            if (c == '.') {
                tiene_decimales = true;
                break;
            }
        }
        // Si se encontró un decimal, imprimir un mensaje
        if (tiene_decimales) {
            cout << "Se encontró un valor decimal en la línea: " << linea << endl;
        }
    }
    archivo.close();
}

int main() {
    // Variables para los parámetros de procesadores y hilos que se leerán del archivo
    int procesadores = 0;
    int hilos_por_procesador = 0;

    // Abrir el archivo para leer los parámetros de procesadores e hilos
    ifstream infile("procesos.dat");
    if (!infile.is_open()) {
        cerr << "Error al abrir el archivo." << endl;
        return 1;
    }
    // Leer las primeras dos líneas del archivo para obtener procesadores y hilos
    string line;
    // Leer la línea de Procesadores
    if (getline(infile, line)) {
        istringstream ss(line);
        string label;
        ss >> label >> procesadores;  // Leer la palabra "Procesadores" y luego el número
    }
    // Leer la línea de Hilos
    if (getline(infile, line)) {
        istringstream ss(line);
        string label;
        ss >> label >> hilos_por_procesador;  // Leer la palabra "Hilos" y luego el número
    }
    infile.close();

    // Validación de hilos_por_procesador para evitar valor 0
    if (hilos_por_procesador <= 0) {
        cerr << "Error: El número de hilos por procesador debe ser mayor a 0." << endl;
        return 1;
    }

    int hilosT = procesadores * hilos_por_procesador;
    cout << "Configuracion:\n";
    cout << "Procesadores: " << procesadores << "\t";
    cout << "Hilos por procesador: " << hilos_por_procesador << "\t";
    cout << "Hilos totales: " << hilosT << endl;

    // Crear el scheduler con el número de hilos disponibles
    MultilevelFeedbackQueueScheduler scheduler(hilosT);
    // Cargar los procesos desde el archivo (saltando las dos primeras líneas ya leídas)
    cargar_procesos_desde_archivo("procesos.dat", scheduler, procesadores, hilosT);

    // Ejecutar los procesos pasando el número de hilos por procesador
    scheduler.ejecutar_procesos(hilos_por_procesador);

    return 0;
}
