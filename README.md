# Análisis experimental del procesamiento paralelo con C++ y MySQL

## 1. Descripción del proyecto

Este proyecto analiza experimentalmente el procesamiento secuencial y paralelo de un conjunto de datos almacenados en MySQL. Se utiliza C++ para implementar y comparar diferentes estrategias de suma sobre un millón de registros numéricos.

Se estudian los tiempos de ejecución, el speedup, la eficiencia y el comportamiento de los métodos al incrementar la cantidad de hilos.

## 2. Objetivo

Evaluar el rendimiento de distintas estrategias de procesamiento mediante la comparación de una implementación secuencial, una implementación paralela con exclusión mutua (`mutex`) y una implementación paralela basada en sumas locales.

## 3. Tecnologías utilizadas

- **C++:** implementación de los algoritmos y medición experimental.
- **MySQL:** almacenamiento y recuperación de los datos.
- **Hilos (`std::thread`):** ejecución concurrente de las operaciones.
- **Mutex (`std::mutex`):** protección del acceso a una variable compartida.
- **CSV:** almacenamiento de las mediciones y los resultados.

## 4. Métodos implementados

### 4.1. Suma secuencial

Recorre los registros de forma secuencial y acumula sus valores en una única variable.

### 4.2. Suma paralela con mutex

Distribuye los datos entre varios hilos y protege cada actualización de la suma compartida mediante un mutex.

### 4.3. Suma paralela con acumuladores locales

Cada hilo calcula un subtotal independiente. Al finalizar los hilos, los subtotales se combinan para obtener el resultado total.

## 5. Configuración experimental

- **Base de datos:** `computacion_paralela`.
- **Tabla:** `numeros`.
- **Cantidad de registros:** 1 000 000.
- **Variable procesada:** `valor`.
- **Cantidad de hilos evaluada:** 1, 2, 4, 8 y 16.
- **Repeticiones por configuración:** 5.
- **Comprobación:** suma total esperada de 500000500000.

## 6. Archivos del repositorio

| Archivo | Descripción |
|---|---|
| `main.cpp` | Código fuente del experimento. |
| `mediciones.csv` | Mediciones individuales de los métodos evaluados. |
| `resumen.csv` | Promedios, desviaciones estándar, speedup y eficiencia. |

## 7. Requisitos

- Compilador compatible con C++17.
- MySQL Server.
- Biblioteca cliente de MySQL/MariaDB para C++.
- Base de datos y tabla creadas con los datos del experimento.

## 8. Configuración de MySQL

Crear la base de datos y la tabla necesarias, y cargar un millón de registros numéricos en la tabla `numeros`. La columna `valor` almacena los valores utilizados en el experimento.

El programa se conecta a la base de datos local con el usuario `root` y obtiene la contraseña desde la variable de entorno `MYSQL_PASSWORD`.

**No se deben publicar contraseñas ni credenciales de acceso en el repositorio.**

## 9. Compilación y ejecución

En un entorno Windows con MSYS2 UCRT64 y la biblioteca cliente instalada, ejecutar en PowerShell:

```powershell
$env:MYSQL_PASSWORD = "TU_CLAVE_REAL"
g++ -std=c++17 -O2 -pthread main.cpp -o main.exe -I"C:/msys64/ucrt64/include/mariadb" -L"C:/msys64/ucrt64/lib" -lmariadb
.\main.exe
```

Reemplazar `TU_CLAVE_REAL` por la contraseña local de MySQL. No compartir ni publicar esa contraseña.

## 10. Métricas de evaluación

- **Tiempo de ejecución:** duración promedio de cada método.
- **Desviación estándar:** variabilidad de los tiempos medidos.
- **Speedup:** relación entre el tiempo secuencial y el tiempo del método evaluado.
- **Eficiencia:** speedup dividido entre la cantidad de hilos y multiplicado por 100.

## 11. Resultados

Los resultados experimentales se encuentran en `mediciones.csv` y `resumen.csv`. Estos archivos permiten elaborar tablas y gráficos comparativos para analizar el comportamiento de cada método.

## 12. Autoría

Proyecto académico de Computación Paralela y Distribuida.

**Universidad Nacional del Altiplano – Puno, Perú.**
