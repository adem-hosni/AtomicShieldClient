

# AtomicShield Client

> Consulta la última versión estable: https://github.com/adem-hosni/AtomicShieldClient/releases/tag/v4

AtomicShield es una solución antitrampas multicapa. Combina un agente de bandeja del sistema orientado al usuario (C#), un motor de inyección nativo (C++) y un conjunto de protecciones en tiempo de ejecución (escaneo de memoria, monitoreo de procesos, anti-depuración) para detectar y prevenir trampas en tiempo real.

## Descripción General de la Arquitectura

```
┌──────────────────────────────────────────────────────────────────┐
│                        AtomicAgent (C#)                          │
│  - Agente de bandeja con panel WebView2                          │
│  - Cliente de API REST cifrado para autenticación/estado         │
│  - Descarga y ejecuta EngineLoader vía pipe nombrada             │
└──────────────┬────────────────────────────────────────┬──────────┘
               │ pipe nombrada                          │
               ▼                                        ▼
┌──────────────────────────────┐    ┌──────────────────────────────┐
│     EngineLoader (C#)        │    │   RuntimeLoader (C++)        │
│  - Servidor de pipe nombrada │    │  - Servidor de pipe nombrada │
│  - InProcessManualMapper     │    │  - Mapper SelfMapModule      │
│  - Mapea la DLL de AtomicEngine│   │  - Mapea la DLL de AtomicEngine│
└──────────────┬───────────────┘    └──────────────┬───────────────┘
               │ mapeo manual                      │ mapeo manual
               └─────────────────┬─────────────────┘
                                 ▼
┌───────────────────────────────────────────────────────────────────┐
│                     AtomicEngine (DLL C++)                        │
│  - Red cifrada WebSocket + HTTP                                  │
│  - 12 métodos anti-depuración                                    │
│  - Protección de Procesos (enumeración de handles)               │
│  - Protección Heurística (escaneo de memoria xxHash)             │
│  - Protección de Mapeo Manual (detección de cabecera PE)         │
│  - Informes de fallo / Captura de pantalla / HWID                │
│  - Escaneo de drivers en lista negra (~40 drivers)               │
└───────────────────────────────────────────────────────────────────┘
```

## Componentes

| Componente | Lenguaje | Descripción |
|-----------|----------|-------------|
| [`AtomicAgent/`](AtomicAgent/) | C# (.NET 8, WebView2) | Agente de bandeja del sistema con panel HTML/JS WebView2, cliente de API cifrado, IPC del cargador del motor |
| [`AtomicEngine/`](AtomicEngine/) | DLL C++ | Motor antitrampas principal: protecciones, anti-depuración, red, HWID, informes de fallo |
| [`EngineLoader/`](EngineLoader/) | Consola C# | Servidor de pipe nombrada que mapea manualmente la DLL del motor en su propio proceso |
| [`RuntimeLoader/`](RuntimeLoader/) | EXE C++ | Cargador nativo alternativo con pipe nombrada + mapeo manual |
| [`Shared/`](Shared/) | Lib C++ | Utilidades compartidas: AES-256-CBC, Base64, registro de eventos (logging), detección de procesos |
| [`SharedLoader/`](SharedLoader/) | Compartido C# | Clase de registro de eventos compartida para ambos proyectos C# |

## Cadena de Carga

1. **AtomicAgent** se inicia (inicio automático mediante Programador de Tareas)
2. El agente verifica la conectividad enviando un ping al servidor API
3. El agente descarga la DLL cifrada del motor desde la API y la descifra
4. El agente escribe `EngineLoader.exe` en `Program Files\AtomicShield\AtomicSvc.exe`
5. El agente ejecuta el cargador con elevación de UAC (`runas`)
6. El agente abre un cliente de pipe nombrada (`\\.\pipe\AtomicPipe`) y envía los bytes de la DLL del motor
7. **EngineLoader** recibe la DLL y utiliza `InProcessManualMapper` para mapearla dentro de su proceso
8. Se ejecuta `DllMain` de **AtomicEngine**: suspende hilos, anti-depuración, inicialización de red, bucle de pulsos
9. El motor se conecta mediante WebSocket a `ws://<SERVER_ADDRESS>` y envía señales de vida (heartbeats)

## Características Principales

- **Anti-Depuración:** 12 métodos de detección (puntos de interrupción de HW, banderas PEB, VEH, depurador de kernel, escaneo de procesos/ventanas, banderas de heap, puerto de depuración, excepción de CloseHandle)
- **Protección de Procesos:** Enumeración de handles del sistema: detecta cualquier proceso con un handle abierto a FiveM
- **Protección Heurística:** Escaneo de regiones de memoria basado en xxHash para firmas de trampas
- **Protección de Mapeo Manual:** Detección de cabeceras PE en regiones de memoria no estándar
- **Drivers en Lista Negra:** Escanea ~40 drivers maliciosos conocidos
- **Recopilación de HWID:** CPU, GPU (NVML/WMI), discos, SMBIOS, TPM (PowerShell), ID de Steam
- **Red Cifrada:** AES-256-CBC con 8 pares de clave/IV rotativos
- **Captura de Pantalla:** Captura GDI+ de la ventana de FiveM
- **Informes de Fallo:** Manejador SEH → POST HTTPS a `<SERVER_ADDRESS>/anticheat/crash-report`
- **Firma de Código:** Verificación de firma Authenticode + catálogo mediante `WinVerifyTrust`
- **Ofuscación de Cadenas:** skCrypter en todo el código C++

## Prerrequisitos de Compilación

- Visual Studio 2022
- .NET 8 SDK
- Herramientas Clang C++ (`clang-cl`)
- UPX (opcional, para compresión)
- Certificado de firma de código (opcional)

## Compilación

Consulta [docs/BUILD.md](docs/BUILD.md) para obtener instrucciones detalladas de compilación.

## Documentación

Consulta el directorio [docs/](docs/):

- [Architecture](docs/ARCHITECTURE.md) — Arquitectura del sistema y flujo de datos
- [AtomicAgent](docs/AtomicAgent.md) — Documentación del agente C#
- [AtomicEngine](docs/AtomicEngine.md) — Documentación del motor antitrampas C++
- [EngineLoader](docs/EngineLoader.md) — Documentación del cargador C#
- [RuntimeLoader](docs/RuntimeLoader.md) — Documentación del cargador C++
- [Shared Library](docs/Shared.md) — Documentación del código compartido
- [Network Protocol](docs/NETWORK_PROTOCOL.md) — Protocolo API y WebSocket
- [Build Process](docs/BUILD.md) — Compilación desde el código fuente
- [Security Features](docs/SECURITY.md) — Detalles de los mecanismos de seguridad

## Licencia

MIT — Copyright (c) 2024 Hyper

## Contacto

- **Correo electrónico:** hosniadem400@gmail.com y aifaouiameen@gmail.com

## Repositorios Relacionados

- [AtomicShield Server](https://github.com/adem-hosni/AtomicShieldServer) — API backend y servidor WebSocket
- [AtomicShield Platform](https://github.com/adem-hosni/atomicshield-platform) — Interfaz de usuario del panel de administración
