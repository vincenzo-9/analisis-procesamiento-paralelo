
#include <mysql.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using namespace chrono;

using Entero = long long;

const int REPETICIONES = 5;
const int REPETICIONES_INTERNAS = 10;

struct Resultado {
    string metodo;
    int hilos;
    int repeticion;
    double tiempo_ms;
};

struct Resumen {
    string metodo;
    int hilos;
    double promedio_ms;
    double desviacion_ms;
    double speedup;
    double eficiencia;
};

// ------------------------------------------------------
// CARGA DE DATOS DESDE MYSQL
// ------------------------------------------------------

vector<Entero> cargarDatosMySQL() {
    MYSQL* conexion = mysql_init(nullptr);

    if (conexion == nullptr) {
        throw runtime_error("No se pudo inicializar MySQL.");
    }

    const char* clave = getenv("MYSQL_PASSWORD");

    if (clave == nullptr || clave[0] == '\0') {
        mysql_close(conexion);
        throw runtime_error(
            "Configura la variable de entorno MYSQL_PASSWORD."
        );
    }

    if (mysql_real_connect(
            conexion,
            "localhost",
            "root",
            clave,
            "computacion_paralela",
            3306,
            nullptr,
            0) == nullptr) {
        string error = mysql_error(conexion);
        mysql_close(conexion);
        throw runtime_error("Error de conexión MySQL: " + error);
    }

    const char* consulta =
        "SELECT valor FROM numeros ORDER BY id";

    if (mysql_query(conexion, consulta) != 0) {
        string error = mysql_error(conexion);
        mysql_close(conexion);
        throw runtime_error("Error al consultar datos: " + error);
    }

    MYSQL_RES* resultado = mysql_store_result(conexion);

    if (resultado == nullptr) {
        string error = mysql_error(conexion);
        mysql_close(conexion);
        throw runtime_error("Error al recuperar resultados: " + error);
    }

    vector<Entero> datos;
    MYSQL_ROW fila;

    while ((fila = mysql_fetch_row(resultado)) != nullptr) {
        if (fila[0] != nullptr) {
            datos.push_back(stoll(fila[0]));
        }
    }

    mysql_free_result(resultado);
    mysql_close(conexion);

    return datos;
}

// ------------------------------------------------------
// SUMA SECUENCIAL
// ------------------------------------------------------

Entero sumaSecuencial(const vector<Entero>& datos) {
    Entero suma = 0;

    for (Entero valor : datos) {
        suma += valor;
    }

    return suma;
}

// ------------------------------------------------------
// SUMA PARALELA CON MUTEX
// ------------------------------------------------------

Entero sumaMutex(const vector<Entero>& datos, int cantidadHilos) {
    mutex cerrojo;
    Entero sumaCompartida = 0;

    vector<thread> hilos;
    size_t n = datos.size();

    for (int t = 0; t < cantidadHilos; ++t) {
        size_t inicio = n * t / cantidadHilos;
        size_t fin = n * (t + 1) / cantidadHilos;

        hilos.emplace_back([&, inicio, fin]() {
            for (size_t i = inicio; i < fin; ++i) {
                lock_guard<mutex> bloqueo(cerrojo);
                sumaCompartida += datos[i];
            }
        });
    }

    for (thread& hilo : hilos) {
        hilo.join();
    }

    return sumaCompartida;
}

// ------------------------------------------------------
// SUMA PARALELA CON ACUMULADORES LOCALES
// ------------------------------------------------------

Entero sumaLocal(const vector<Entero>& datos, int cantidadHilos) {
    vector<Entero> sumasLocales(cantidadHilos, 0);
    vector<thread> hilos;

    size_t n = datos.size();

    for (int t = 0; t < cantidadHilos; ++t) {
        size_t inicio = n * t / cantidadHilos;
        size_t fin = n * (t + 1) / cantidadHilos;

        hilos.emplace_back([&, t, inicio, fin]() {
            Entero subtotal = 0;

            for (size_t i = inicio; i < fin; ++i) {
                subtotal += datos[i];
            }

            sumasLocales[t] = subtotal;
        });
    }

    for (thread& hilo : hilos) {
        hilo.join();
    }

    Entero sumaTotal = 0;

    for (Entero subtotal : sumasLocales) {
        sumaTotal += subtotal;
    }

    return sumaTotal;
}

// ------------------------------------------------------
// MEDICIÓN DE TIEMPOS
// ------------------------------------------------------

template <typename Funcion>
double medirPromedio(Funcion funcion) {
    auto inicio = high_resolution_clock::now();

    for (int i = 0; i < REPETICIONES_INTERNAS; ++i) {
        funcion();
    }

    auto fin = high_resolution_clock::now();

    double total = duration<double, milli>(fin - inicio).count();

    return total / REPETICIONES_INTERNAS;
}

double calcularPromedio(const vector<double>& tiempos) {
    double suma = 0.0;

    for (double tiempo : tiempos) {
        suma += tiempo;
    }

    return suma / tiempos.size();
}

double calcularDesviacion(const vector<double>& tiempos) {
    double promedio = calcularPromedio(tiempos);
    double sumaCuadrados = 0.0;

    for (double tiempo : tiempos) {
        double diferencia = tiempo - promedio;
        sumaCuadrados += diferencia * diferencia;
    }

    return sqrt(sumaCuadrados / tiempos.size());
}

// ------------------------------------------------------
// EXPORTACIÓN DE MEDICIONES Y RESUMEN
// ------------------------------------------------------

void guardarMediciones(const vector<Resultado>& mediciones) {
    ofstream archivo("mediciones.csv");

    if (!archivo.is_open()) {
        throw runtime_error("No se pudo crear mediciones.csv");
    }

    archivo << "metodo,hilos,repeticion,tiempo_promedio_ms\n";
    archivo << fixed << setprecision(6);

    for (const Resultado& r : mediciones) {
        archivo << r.metodo << ','
                << r.hilos << ','
                << r.repeticion << ','
                << r.tiempo_ms << '\n';
    }
}

vector<Resumen> calcularResumen(
    const vector<Resultado>& mediciones,
    double tiempoSecuencial) {

    vector<Resumen> resumen;

    vector<pair<string, int>> configuraciones;
    configuraciones.push_back({"Secuencial", 1});

    for (int hilos : {1, 2, 4, 8, 16}) {
        configuraciones.push_back({"Mutex", hilos});
        configuraciones.push_back({"SumaLocal", hilos});
    }

    for (const auto& configuracion : configuraciones) {
        vector<double> tiempos;

        for (const Resultado& r : mediciones) {
            if (r.metodo == configuracion.first &&
                r.hilos == configuracion.second) {
                tiempos.push_back(r.tiempo_ms);
            }
        }

        if (tiempos.empty()) {
            continue;
        }

        double promedio = calcularPromedio(tiempos);
        double desviacion = calcularDesviacion(tiempos);
        double speedup = tiempoSecuencial / promedio;
        double eficiencia =
            speedup / configuracion.second * 100.0;

        resumen.push_back({
            configuracion.first,
            configuracion.second,
            promedio,
            desviacion,
            speedup,
            eficiencia
        });
    }

    return resumen;
}

void guardarResumen(const vector<Resumen>& resumen) {
    ofstream archivo("resumen.csv");

    if (!archivo.is_open()) {
        throw runtime_error("No se pudo crear resumen.csv");
    }

    archivo << "metodo,hilos,tiempo_promedio_ms,"
            << "desviacion_ms,speedup,eficiencia_porcentaje\n";

    archivo << fixed << setprecision(6);

    for (const Resumen& r : resumen) {
        archivo << r.metodo << ','
                << r.hilos << ','
                << r.promedio_ms << ','
                << r.desviacion_ms << ','
                << r.speedup << ','
                << r.eficiencia << '\n';
    }
}

// ------------------------------------------------------
// PROGRAMA PRINCIPAL
// ------------------------------------------------------

int main() {
    try {
        cout << "========================================\n";
        cout << " ANALISIS DE PROCESAMIENTO PARALELO\n";
        cout << "========================================\n\n";

        cout << "Conectando con MySQL...\n";

        vector<Entero> datos = cargarDatosMySQL();

        if (datos.empty()) {
            throw runtime_error("La consulta no devolvió datos.");
        }

        cout << "Datos cargados: " << datos.size() << '\n';

        Entero sumaEsperada = sumaSecuencial(datos);

        cout << "Suma de comprobacion: "
             << sumaEsperada << "\n\n";

        vector<Resultado> mediciones;

        cout << "Midiendo metodo secuencial...\n";

        for (int repeticion = 1;
             repeticion <= REPETICIONES;
             ++repeticion) {

            Entero suma = 0;

            double tiempo = medirPromedio([&]() {
                suma = sumaSecuencial(datos);
            });

            if (suma != sumaEsperada) {
                throw runtime_error(
                    "La suma secuencial no coincide."
                );
            }

            mediciones.push_back({
                "Secuencial", 1, repeticion, tiempo
            });
        }

        for (int hilos : {1, 2, 4, 8, 16}) {
            cout << "Probando con " << hilos
                 << " hilo(s)...\n";

            for (int repeticion = 1;
                 repeticion <= REPETICIONES;
                 ++repeticion) {

                Entero suma = 0;

                double tiempo = medirPromedio([&]() {
                    suma = sumaMutex(datos, hilos);
                });

                if (suma != sumaEsperada) {
                    throw runtime_error(
                        "La suma con mutex no coincide."
                    );
                }

                mediciones.push_back({
                    "Mutex", hilos, repeticion, tiempo
                });

                tiempo = medirPromedio([&]() {
                    suma = sumaLocal(datos, hilos);
                });

                if (suma != sumaEsperada) {
                    throw runtime_error(
                        "La suma local no coincide."
                    );
                }

                mediciones.push_back({
                    "SumaLocal", hilos, repeticion, tiempo
                });
            }
        }

        guardarMediciones(mediciones);

        vector<double> tiemposSecuenciales;

        for (const Resultado& r : mediciones) {
            if (r.metodo == "Secuencial") {
                tiemposSecuenciales.push_back(r.tiempo_ms);
            }
        }

        double promedioSecuencial =
            calcularPromedio(tiemposSecuenciales);

        vector<Resumen> resumen =
            calcularResumen(mediciones, promedioSecuencial);

        guardarResumen(resumen);

        cout << "\nMetodo      Hilos   Promedio(ms)"
             << "     Desv.(ms)        Speedup"
             << "     Eficiencia(%)\n";

        cout << "------------------------------------------------"
             << "------------------\n";

        cout << fixed << setprecision(6);

        for (const Resumen& r : resumen) {
            cout << left << setw(12) << r.metodo
                 << setw(8) << r.hilos
                 << setw(16) << r.promedio_ms
                 << setw(16) << r.desviacion_ms
                 << setw(14) << r.speedup
                 << r.eficiencia << '\n';
        }

        cout << "\n========================================\n";
        cout << "EXPERIMENTO FINALIZADO\n";
        cout << "========================================\n";
        cout << "Archivos generados:\n";
        cout << "- mediciones.csv\n";
        cout << "- resumen.csv\n";
        cout << "Suma correcta: " << sumaEsperada << '\n';

    } catch (const exception& e) {
        cerr << "\nERROR: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
