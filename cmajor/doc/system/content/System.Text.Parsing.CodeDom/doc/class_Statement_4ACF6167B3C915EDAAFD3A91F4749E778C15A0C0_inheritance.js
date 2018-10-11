// JavaScript source code for drawing class inheritance diagrams and concept refinement diagrams

function pick(level, diagramNodes) {
    var levelNodes = [];
    var n = diagramNodes.length;
    for (var i = 0; i < n; ++i) {
        var c = diagramNodes[i];
        if (c.level == level) {
            levelNodes.push(c);
        }
    }
    return levelNodes;
}

function createDiagramNodeElements(levelNodes, maxTextWidth, maxTextHeight) {
    var textDimensions = { width: maxTextWidth, height: maxTextHeight };
    var n = levelNodes.length;
    for (var i = 0; i < n; ++i) {
        var levelNode = levelNodes[i];
        var svg = document.getElementById("inheritance_svg_diagram");
        var rectElement = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
        rectElement.setAttribute("id", levelNode.id + "_rect");
        var linkElement = document.createElementNS('http://www.w3.org/2000/svg', 'a');
        linkElement.setAttribute("href", levelNode.link);
        var textElement = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        linkElement.appendChild(textElement);
        textElement.setAttribute("id", levelNode.id + "_text");
        textElement.innerHTML = levelNode.name;
        svg.appendChild(rectElement);
        svg.appendChild(linkElement);
        var bb = textElement.getBBox();
        var textWidth = bb.width;
        var textHeight = bb.height;
        levelNode.textWidth = textWidth;
        levelNode.textHeight = textHeight;
        if (textWidth > textDimensions.width) {
            textDimensions.width = textWidth;
        }
        if (textHeight > textDimensions.height) {
            textDimensions.height = textHeight;
        }
    }
    return textDimensions;
}

function drawDiagram(diagramNodes) {
    var cont = true;
    var level = 0;
    var yspace = 8;
    var xspace = 8;
    var minRectWidth = 100;
    var minRectHeight = 40;
    var maxTextWidth = 0;
    var maxTextHeight = 0;
    var triangleHeight = 20;
    var triangleWidth = 20;
    var targetHandleHeight = 20;
    var sourceHandleHeight = 40;
    var rectXSpace = 20;
    var horizontalRectYSpace = triangleHeight + targetHandleHeight + sourceHandleHeight;
    var verticalRectYSpace = 20;
    var derivedTriangleHeight = 8;
    var derivedTriangleWidth = 8;
    var widthThreshold = 1800;
    var allLevelNodes = [];
    while (cont) {
        var levelNodes = pick(level, diagramNodes);
        var n = levelNodes.length;
        cont = n > 0;
        if (cont) {
            var textDimensions = createDiagramNodeElements(levelNodes, maxTextWidth, maxTextHeight);
            if (textDimensions.width > maxTextWidth) {
                maxTextWidth = textDimensions.width;
            }
            if (textDimensions.height > maxTextHeight) {
                maxTextHeight = textDimensions.height;
            }
            allLevelNodes.push(levelNodes);
            ++level;
        }
    }
    var maxLevel = level;
    var rectWidth = Math.max(minRectWidth, maxTextWidth + 2 * xspace);
    var rectHeight = Math.max(minRectHeight, maxTextHeight + 2 * yspace);
    var totalWidth = 0;
    var totalHeight = 0;
    var horizontalDirection = 0;
    var verticalDirection = 1;
    var levelDirection = [];
    var levelHeight = [];
    var prevW = 0;
    for (level = 0; level < maxLevel; ++level) {
        var levelNodes = allLevelNodes[level];
        var n = levelNodes.length;
        var w = n * (rectWidth + rectXSpace);
        var h = rectHeight + horizontalRectYSpace;
        if (w < widthThreshold) {
            levelDirection.push(horizontalDirection);
            if (w > totalWidth) {
                totalWidth = w;
            }
        }
        else {
            w = prevW + rectWidth + rectXSpace;
            h = n * (rectHeight + verticalRectYSpace);
            levelDirection.push(verticalDirection);
            totalWidth += w;
        }
        totalHeight += h;
        levelHeight.push(h);
        prevW = w;
    }
    var svg = document.getElementById("inheritance_svg_diagram");
    svg.setAttribute("width", totalWidth.toString());
    svg.setAttribute("height", totalHeight.toString());
    var prevRectY = 0;
    var prevRectX = 0;
    var prevHandleX2 = -1;
    var prevHandleY2 = -1;
    var prevY = 0;
    for (level = 0; level < maxLevel; ++level) {
        var direction = levelDirection[level];
        var levelNodes = allLevelNodes[level];
        var n = levelNodes.length;
        var rectY = prevY;
        prevY += levelHeight[level];
        var rectX = (totalWidth / n - rectWidth) / 2;
        var minHandleX = Number.MAX_SAFE_INTEGER;
        var maxHandleX = 0;
        var handleY = 0;
        for (var i = 0; i < n; ++i) {
            var levelNode = levelNodes[i];
            var textWidth = levelNode.textWidth;
            var textHeight = levelNode.textHeight;
            if (direction == horizontalDirection) {
                rectX = (totalWidth / n - rectWidth) / 2 + i * (rectWidth + rectXSpace);
            }
            else if (direction == verticalDirection) {
                rectX = prevRectX + (rectWidth + rectXSpace);
                rectY = prevRectY + horizontalRectYSpace + i * (rectHeight + verticalRectYSpace);
            }
            var textX = rectX + (rectWidth - textWidth) / 2;
            var textY = (rectY + rectHeight - yspace / 2) - (rectHeight - textHeight) / 2;
            var rectElement = document.getElementById(levelNode.id + "_rect");
            rectElement.setAttribute("x", rectX.toString());
            rectElement.setAttribute("y", rectY.toString());
            rectElement.setAttribute("width", rectWidth.toString());
            rectElement.setAttribute("height", rectHeight.toString());
            var fillColor = "white";
            if (levelNode.subject) {
                fillColor = "floralWhite";
            }
            rectElement.setAttribute("fill", fillColor);
            rectElement.setAttribute("stroke", "black");
            var textElement = document.getElementById(levelNode.id + "_text");
            textElement.setAttribute("x", textX.toString());
            textElement.setAttribute("y", textY.toString());
            if (level < maxLevel - 1) {
                var triangleElement = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
                var tipX = rectX + rectWidth / 2;
                var tipY = rectY + rectHeight;
                var leftX = tipX - triangleWidth / 2;
                var leftY = rectY + rectHeight + triangleHeight;
                var rightX = tipX + triangleWidth / 2;
                var rightY = rectY + rectHeight + triangleHeight;
                triangleElement.setAttribute("points",
                    tipX.toString() + "," + tipY.toString() + " " +
                    leftX.toString() + "," + leftY.toString() + " " +
                    rightX.toString() + "," + rightY.toString());
                triangleElement.setAttribute("fill", "white");
                triangleElement.setAttribute("stroke", "black");
                svg.appendChild(triangleElement);
                var targetHandleElement = document.createElementNS('http://www.w3.org/2000/svg', 'line');
                var handleX1 = tipX;
                var handleY1 = tipY + triangleHeight;
                var handleX2 = tipX;
                var handleY2 = tipY + triangleHeight + targetHandleHeight;
                targetHandleElement.setAttribute("x1", handleX1.toString());
                targetHandleElement.setAttribute("y1", handleY1.toString());
                targetHandleElement.setAttribute("x2", handleX2.toString());
                targetHandleElement.setAttribute("y2", handleY2.toString());
                targetHandleElement.setAttribute("stroke", "black");
                svg.appendChild(targetHandleElement);
                prevHandleX2 = handleX1;
                prevHandleY2 = handleY1;
                if (handleX1 < minHandleX) {
                    minHandleX = handleX1;
                    handleY = handleY2;
                }
                if (handleX1 > maxHandleX) {
                    maxHandleX = handleX1;
                    handleY = handleY2;
                }
            } else if (level == maxLevel - 1 && levelNode.hasDerivedNodes) {
                var derivedTriangleElement = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
                var cornerX = rectX + rectWidth;
                var cornerY = rectY + rectHeight;
                var verticalX = rectX + rectWidth;
                var verticalY = rectY + rectHeight - derivedTriangleHeight;
                var horizontalX = rectX + rectWidth - derivedTriangleWidth;
                var horizontalY = rectY + rectHeight;
                derivedTriangleElement.setAttribute("points",
                    cornerX.toString() + "," + cornerY.toString() + " " +
                    verticalX.toString() + "," + verticalY.toString() + " " +
                    horizontalX.toString() + "," + horizontalY.toString());
                derivedTriangleElement.setAttribute("fill", "black");
                svg.appendChild(derivedTriangleElement);
            }
            if (level > 0 && direction == horizontalDirection) {
                var sourceHandleElement = document.createElementNS('http://www.w3.org/2000/svg', 'line');
                var handleX1 = rectX + rectWidth / 2;
                var handleY1 = rectY;
                var handleX2 = rectX + rectWidth / 2;
                var handleY2 = rectY - sourceHandleHeight;
                sourceHandleElement.setAttribute("x1", handleX1.toString());
                sourceHandleElement.setAttribute("y1", handleY1.toString());
                sourceHandleElement.setAttribute("x2", handleX2.toString());
                sourceHandleElement.setAttribute("y2", handleY2.toString());
                sourceHandleElement.setAttribute("stroke", "black");
                svg.appendChild(sourceHandleElement);
                if (handleX1 < minHandleX) {
                    minHandleX = handleX1;
                    handleY = handleY2;
                }
                if (handleX1 > maxHandleX) {
                    maxHandleX = handleX1;
                    handleY = handleY2;
                }
            }
            else if (level > 0 && direction == verticalDirection) {
                var sourceHandleElement = document.createElementNS('http://www.w3.org/2000/svg', 'line');
                var handleX1 = rectX;
                var handleY1 = rectY + rectHeight / 2;
                var handleX2 = rectX - rectWidth / 2 - rectXSpace;
                var handleY2 = rectY + rectHeight / 2;
                sourceHandleElement.setAttribute("x1", handleX1.toString());
                sourceHandleElement.setAttribute("y1", handleY1.toString());
                sourceHandleElement.setAttribute("x2", handleX2.toString());
                sourceHandleElement.setAttribute("y2", handleY2.toString());
                sourceHandleElement.setAttribute("stroke", "black");
                svg.appendChild(sourceHandleElement);
                if (prevHandleX2 != -1 && prevHandleY2 != -1) {
                    var connectorHandleElement = document.createElementNS('http://www.w3.org/2000/svg', 'line');
                    connectorHandleElement.setAttribute("x1", handleX2.toString());
                    connectorHandleElement.setAttribute("y1", handleY2.toString());
                    connectorHandleElement.setAttribute("x2", prevHandleX2.toString());
                    connectorHandleElement.setAttribute("y2", prevHandleY2.toString());
                    connectorHandleElement.setAttribute("stroke", "black");
                    svg.appendChild(connectorHandleElement);
                }
                prevHandleX2 = handleX2
                prevHandleY2 = handleY2;
            }
        }
        if (minHandleX < maxHandleX && direction == horizontalDirection) {
            var hlineElement = document.createElementNS('http://www.w3.org/2000/svg', 'line');
            hlineElement.setAttribute("x1", minHandleX.toString());
            hlineElement.setAttribute("y1", handleY.toString());
            hlineElement.setAttribute("x2", maxHandleX.toString());
            hlineElement.setAttribute("y2", handleY.toString());
            hlineElement.setAttribute("stroke", "black");
            svg.appendChild(hlineElement);
        }
        prevRectY = rectY;
        prevRectX = rectX;
    }
}

function drawInheritanceDiagram() {
    var diagramNodes = [
        { name: "System.Text.Parsing.CodeDom.DomObject", id: "diagram_node_0", level: 0, subject: false, hasDerivedNodes: true, link: "../../System.Text.Parsing.CodeDom/doc/class_DomObject_A6EA469160F8FD3B17826E7729430E5F3D07C4DF.html" },
        { name: "System.Text.Parsing.CodeDom.Statement", id: "diagram_node_1", level: 1, subject: true, hasDerivedNodes: true, link: "../../System.Text.Parsing.CodeDom/doc/class_Statement_4ACF6167B3C915EDAAFD3A91F4749E778C15A0C0.html" },
        { name: "System.Text.Parsing.CodeDom.AssertStatement", id: "diagram_node_2", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_AssertStatement_D3CF5F3C391681B790B0E9E570953DA4FA666696.html" },
        { name: "System.Text.Parsing.CodeDom.AssignmentStatement", id: "diagram_node_3", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_AssignmentStatement_CD891DADD31EA0AD502EF64C3B188877E8D8CE3D.html" },
        { name: "System.Text.Parsing.CodeDom.BreakStatement", id: "diagram_node_5", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_BreakStatement_4B85AA125DCD9CEF1305B090C2F86B174A514580.html" },
        { name: "System.Text.Parsing.CodeDom.CaseStatement", id: "diagram_node_6", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_CaseStatement_C59D9EE2EB215BB01443FA26C029108CF42800A1.html" },
        { name: "System.Text.Parsing.CodeDom.CompoundStatement", id: "diagram_node_10", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_CompoundStatement_D64148F52F69E76B5603068E1B519F54CC99F8A9.html" },
        { name: "System.Text.Parsing.CodeDom.ConstructionStatement", id: "diagram_node_12", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_ConstructionStatement_0E8F6D33B33CABF85AEE993CBB0F609D7E941AEA.html" },
        { name: "System.Text.Parsing.CodeDom.ContinueStatement", id: "diagram_node_11", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_ContinueStatement_42B62A34F11C57D46CBD9B9E4C6DC070CA7E1CFC.html" },
        { name: "System.Text.Parsing.CodeDom.DefaultStatement", id: "diagram_node_4", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_DefaultStatement_431BBB02B62D93DF15B8BA112275D930065BD749.html" },
        { name: "System.Text.Parsing.CodeDom.DeleteStatement", id: "diagram_node_9", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_DeleteStatement_CFF57A1EB43152DAED3E95873037A4F83E2AAF0A.html" },
        { name: "System.Text.Parsing.CodeDom.DestroyStatement", id: "diagram_node_13", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_DestroyStatement_E2AF4A1B38D4AE66FCF9EF639F0D8309CE050550.html" },
        { name: "System.Text.Parsing.CodeDom.DoStatement", id: "diagram_node_8", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_DoStatement_2DE84CAC1D895D9020F0342FAD7D807D41BC6D0E.html" },
        { name: "System.Text.Parsing.CodeDom.EmptyStatement", id: "diagram_node_14", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_EmptyStatement_6FB774F9105B54DBC81384D86B6519B51F8CEC1A.html" },
        { name: "System.Text.Parsing.CodeDom.ExpressionStatement", id: "diagram_node_15", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_ExpressionStatement_F01731B3CB572136FE603DE87C8C465F5B79ED54.html" },
        { name: "System.Text.Parsing.CodeDom.ForStatement", id: "diagram_node_7", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_ForStatement_2C9CB31D518BF6F5E055FFE544F154898DB891FC.html" },
        { name: "System.Text.Parsing.CodeDom.GotoCaseStatement", id: "diagram_node_16", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_GotoCaseStatement_54B0F8581DE0382A086C8C1EC6A19E93889D566D.html" },
        { name: "System.Text.Parsing.CodeDom.GotoDefaultStatement", id: "diagram_node_17", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_GotoDefaultStatement_A0F05081B3A8A0CF31277B3E9AB698FF523E7B9F.html" },
        { name: "System.Text.Parsing.CodeDom.GotoStatement", id: "diagram_node_19", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_GotoStatement_423A80C64905BCD7885A3433F9FE33D8BB37DF7E.html" },
        { name: "System.Text.Parsing.CodeDom.IfStatement", id: "diagram_node_20", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_IfStatement_C1EF6483EAB3986BA6A34F9E4C9ED4C0F2F824DE.html" },
        { name: "System.Text.Parsing.CodeDom.RangeForStatement", id: "diagram_node_22", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_RangeForStatement_785371970BF070388FB222CA14A1494243EB9D3E.html" },
        { name: "System.Text.Parsing.CodeDom.ReturnStatement", id: "diagram_node_23", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_ReturnStatement_FEC70A987D7361EC167E7C772D29550ABF5D7750.html" },
        { name: "System.Text.Parsing.CodeDom.SwitchStatement", id: "diagram_node_18", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_SwitchStatement_18D2C0BB3BA2134D050B02EA375A462847662B6E.html" },
        { name: "System.Text.Parsing.CodeDom.ThrowStatement", id: "diagram_node_21", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_ThrowStatement_6216EF486931E535CE6CDEE33F4D2E7A53BE76B7.html" },
        { name: "System.Text.Parsing.CodeDom.TryStatement", id: "diagram_node_24", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_TryStatement_1132D26D95ED5027A0CFA7B5E46039A7A645EE96.html" },
        { name: "System.Text.Parsing.CodeDom.WhileStatement", id: "diagram_node_25", level: 2, subject: false, hasDerivedNodes: false, link: "../../System.Text.Parsing.CodeDom/doc/class_WhileStatement_73BA811E542035F6EB0CDBB6874FA3BEF573C0F3.html" }];
    drawDiagram(diagramNodes);
}
