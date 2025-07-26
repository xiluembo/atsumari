import QtQuick
import QtQuick3D

Item {
    id: root
    width: 800
    height: 600

    // Config properties - use defaults for now

    // Common Props
    property color baseColor: Qt.rgba(0.352, 0.27, 0.517, 1.0)
    property color ambientColor: Qt.rgba(0.0, 0.0, 0.156, 1.0)
    property int rotationInterval: 3000
    property color lightColor: Qt.rgba(1.0, 1.0, 1.0, 1.0)
    property real brightness: 1.0

    // for Principled
    property real roughness: 0.60
    property real metalness: 0.00
    property real refraction: 1.60

    // for Specular Glossy
    property color specularColor: Qt.rgba(0.266, 0.341, 0.517, 1.0)
    property real glossiness: 0.60
    property bool useSpecularGlossyMaterial : true
    property var emoteList: []
    
    // Decoration path property
    property string decorationPath: "qrc:/decoration/kata_deco.png"

    property var cameraRotationTarget: Qt.vector3d(180, 360, 0)
    property var specMaterial: SpecularGlossyMaterial {
        albedoColor: root.baseColor
        specularColor: root.specularColor
        glossiness: root.glossiness
    }

    property var princMaterial: PrincipledMaterial {
        baseColor: root.baseColor
        roughness: root.roughness
        metalness: root.metalness
        indexOfRefraction: root.refraction
    }

    // The root scene
    Node {
        id: standAloneScene

        Model {
            id: sphere
            source: "#Sphere"
            scale: Qt.vector3d(1, 1, 1)
            materials: [ root.useSpecularGlossyMaterial ? root.specMaterial : root.princMaterial ]
        }

        // Main Camera
        Node {
            id: cameraAndLightNode
            DirectionalLight {
                ambientColor: root.ambientColor
                color: root.lightColor
                brightness: root.brightness
            }

            DirectionalLight {
                ambientColor: root.ambientColor
                color: root.lightColor
                brightness: root.brightness
                eulerRotation.x: 180
            }

            PerspectiveCamera {
                id: cameraMain
                z: 150
                fieldOfView: 45.0
                clipNear: 0.1
                clipFar: 1000.0
            }

            ParallelAnimation {
                id: rotationAnimation
                running: true
                NumberAnimation {
                    target: cameraAndLightNode
                    property: "eulerRotation.x"
                    duration: root.rotationInterval
                    to: root.cameraRotationTarget.x
                }
                NumberAnimation {
                    target: cameraAndLightNode
                    property: "eulerRotation.y"
                    duration: root.rotationInterval
                    to: root.cameraRotationTarget.y
                }
                NumberAnimation {
                    target: cameraAndLightNode
                    property: "eulerRotation.z"
                    duration: root.rotationInterval
                    to: root.cameraRotationTarget.z
                }
            }
        }
    }

    Timer {
        interval: root.rotationInterval
        running: true
        repeat: true
        onTriggered: {
            root.cameraRotationTarget = Qt.vector3d(
                Math.random() * 360,
                Math.random() * 360,
                Math.random() * 360
            )

            console.log("t:" + root.cameraRotationTarget);

            rotationAnimation.restart();
        }
    }

    Rectangle {
        id: topLeft
        anchors.top: parent.top
        anchors.left: parent.left
        width: parent.width
        height: parent.height
        color: "black"
        border.color: "black"

        View3D {
            id: topLeftView
            anchors.fill: parent
            importScene: standAloneScene
            camera: cameraMain
        }
    }

    // Functions

    function addEmote(emoteUrl, theta, phi, emoteSize) {
        // Add to QML scene for immediate visual feedback
        let radius = 50.02; // Default radius for chat emotes
        let opacity = 0.8; // Default opacity for chat emotes
        
        // Handle random positioning (-1.0 means random position)
        let finalTheta = theta;
        let finalPhi = phi;
        
        if (theta === -1.0 || phi === -1.0) {
            // Generate random spherical coordinates for chat emotes
            finalTheta = Math.random() * 2 * Math.PI;
            finalPhi = Math.acos(2 * Math.random() - 1);
            // Chat emotes have reduced opacity to distinguish from initial emotes
        } else {
            // Icosahedron vertices use slightly smaller radius and full opacity
            radius = 50.01;
            opacity = 1.0;
        }
        
        const x = radius * Math.sin(finalPhi) * Math.cos(finalTheta);
        const y = radius * Math.sin(finalPhi) * Math.sin(finalTheta);
        const z = radius * Math.cos(finalPhi);

        // Rotation angles to make the emote parallel to the surface
        let tilt = finalPhi * (180 / Math.PI);  // Adjust tilt based on phi
        let rotation = finalTheta * (180 / Math.PI);     // Adjust rotation around sphere

        var emote = Qt.createQmlObject(`
            import QtQuick;
            import QtQuick3D;
            Node {
                position: Qt.vector3d(` + x + `, ` + y + `, ` + z + `);
                eulerRotation: Qt.vector3d(` + 0 + `, ` + 0 + `, ` + rotation + `); // apply rotation after tilt
                Model {
                    source: "#Rectangle"
                    scale: Qt.vector3d(` + emoteSize + `, ` + emoteSize + `, ` + emoteSize + `);
                    //position: Qt.vector3d(` + x + `, ` + y + `, ` + z + `);
                    eulerRotation: Qt.vector3d(` + 0 + `, ` + tilt + `, ` + 0 + `); // first apply tilt

                    materials: [
                        PrincipledMaterial {
                            alphaMode: PrincipledMaterial.Blend
                            baseColorMap: Texture {
                                source: "` + emoteUrl + `"
                            }
                            cullMode: DefaultMaterial.NoCulling
                            opacity: ` + opacity + `
                        }
                    ]
                }
            }
        `, standAloneScene);

        emoteList.push(emote);
    }

    function clearEmotes() {
        for (var i = 0; i < emoteList.length; i++) {
            if (emoteList[i]) {
                emoteList[i].destroy()
            }
        }
        emoteList = []
    }

    // function to add emotes at icosahedron vertices (12 positions)
    function addEmoteAtIcosahedronVertex() {
        // Clear existing emotes first
        clearEmotes();
        
        // Icosahedron vertex positions (theta, phi in degrees)
        const icosahedronVertices = [
            [0.0, 0.0],      // Top vertex
            [0.0, 180.0],    // Bottom vertex
            [0.0, 63.435],   // Upper ring
            [72.0, 63.435],
            [144.0, 63.435],
            [216.0, 63.435],
            [288.0, 63.435],
            [36.0, 116.565], // Lower ring
            [108.0, 116.565],
            [180.0, 116.565],
            [252.0, 116.565],
            [324.0, 116.565]
        ];
        
        // Convert degrees to radians and add emotes using the configured decoration path
        for (let i = 0; i < icosahedronVertices.length; i++) {
            const theta = icosahedronVertices[i][0] * Math.PI / 180;
            const phi = icosahedronVertices[i][1] * Math.PI / 180;
            addEmote(root.decorationPath, theta, phi, 0.25);
        }
    }
} 
