QtApplication {
	name: "Brushy"
	files: [
		"*.cpp",
		"*.h",
	]
	cpp.cxxLanguageVersion: "c++17"
    cpp.includePaths: [sourceDirectory]

    Group {
        files: ["../data/**"]
        fileTags: "qt.core.resource_data"
        Qt.core.resourceSourceBase: "../data/"
        Qt.core.resourcePrefix: "/"
    }

    Qt.qml.importName: "cc.blackquill.janet.Brushy"
    Qt.qml.importVersion: "1.0"

	Depends { name: "Qt"; submodules: ["qml", "quick", "widgets"] }
}
