#ifndef DLL_H
#define DLL_H

#ifdef WIN32

#undef DllPluginManagerExport
#ifdef ExportPluginManagerSymbols
#define DllPluginManagerExport   __declspec( dllexport )
#else
#define DllPluginManagerExport   __declspec( dllimport )
#endif

#undef DllCoreExport
#ifdef ExportCoreSymbols
#define DllCoreExport   __declspec( dllexport )
#else
#define DllCoreExport   __declspec( dllimport )
#endif


#undef DllServerExport
#ifdef ExportServerSymbols
#define DllServerExport   __declspec( dllexport )
#else
#define DllServerExport   __declspec( dllimport )
#endif


#undef DllGuiExport
#ifdef ExportGuiSymbols
#define DllGuiExport   __declspec( dllexport )
#define CTK_WIDGETS_EXPORT __declspec( dllexport )
#define CTK_CORE_EXPORT __declspec( dllexport )
#else

#define DllGuiExport   __declspec( dllimport )
#define CTK_WIDGETS_EXPORT __declspec( dllimport )
#define CTK_CORE_EXPORT __declspec( dllimport )
#endif

#undef DllPluginExport
#ifdef ExportPluginSymbols
#define DllPluginExport   __declspec( dllexport )
#else
#define DllPluginExport   __declspec( dllimport )
#endif

#undef DllPythonExport
#ifdef ExportPythonCoreSymbols
#define DllPythonExport   __declspec( dllexport )
#else
#define DllPythonExport   __declspec( dllimport )
#endif

#else // not WIN32


#undef DllPluginManagerExport
#define DllPluginManagerExport

#undef DllCoreExport
#define DllCoreExport

#define DllServerExport

#define DllGuiExport
#define CTK_WIDGETS_EXPORT
#define CTK_CORE_EXPORT

#undef DllPluginExport
#define DllPluginExport

#undef DllPythonExport
#define DllPythonExport

#endif
#endif // DLL_H
