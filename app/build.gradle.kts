plugins {
    id("com.android.application")
}

android {
    namespace = "es.chiteroman.toramhack"
    compileSdk = 34
    buildToolsVersion = "34.0.0"
    ndkVersion = "26.3.11579264"

    buildFeatures {
        prefab = true
    }

    packaging {
        jniLibs {
            excludes += "**/liblog.so"
            excludes += "**/libshadowhook.so"
        }
    }

    defaultConfig {
        applicationId = "es.chiteroman.toramhack"
        minSdk = 33
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"

        externalNativeBuild {
            cmake {
                abiFilters += "arm64-v8a"

                arguments += "-DANDROID_STL=c++_static"
                arguments += "-DCMAKE_BUILD_TYPE=MinSizeRel"

                cppFlags += "-std=c++20"
                cppFlags += "-fvisibility=hidden"
                cppFlags += "-fvisibility-inlines-hidden"
                cppFlags += "-DkNO_KEYSTONE"
                cppFlags += "-DkITTYMEMORY_DEBUG"
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
        }
    }
}

dependencies {
    implementation("com.bytedance.android:shadowhook:1.0.9")
}

tasks.register("updateModuleProp") {
    doLast {
        val versionName = project.android.defaultConfig.versionName
        val versionCode = project.android.defaultConfig.versionCode

        val modulePropFile = project.rootDir.resolve("module/module.prop")

        var content = modulePropFile.readText()

        content = content.replace(Regex("version=.*"), "version=$versionName")
        content = content.replace(Regex("versionCode=.*"), "versionCode=$versionCode")

        modulePropFile.writeText(content)
    }
}


tasks.register("copyFiles") {
    dependsOn("updateModuleProp")

    doLast {
        val moduleFolder = project.rootDir.resolve("module")
        val soDir =
            project.layout.buildDirectory.get().asFile.resolve("intermediates/stripped_native_libs/release/stripReleaseDebugSymbols/out/lib")

        soDir.walk().filter { it.isFile && it.extension == "so" }.forEach { soFile ->
            val abiFolder = soFile.parentFile.name
            val destination = moduleFolder.resolve("zygisk/$abiFolder.so")
            soFile.copyTo(destination, overwrite = true)
        }
    }
}

tasks.register<Zip>("zip") {
    dependsOn("copyFiles")

    archiveFileName.set("ToramHack_${project.android.defaultConfig.versionName}.zip")
    destinationDirectory.set(project.rootDir.resolve("out"))

    from(project.rootDir.resolve("module"))
}

afterEvaluate {
    tasks["assembleRelease"].finalizedBy("updateModuleProp", "copyFiles", "zip")
}