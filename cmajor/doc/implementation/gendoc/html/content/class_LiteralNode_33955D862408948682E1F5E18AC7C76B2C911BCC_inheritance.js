// JavaScript source code for drawing class inheritance diagrams

function pick(level, classes) {
    var levelClasses = [];
    var n = classes.length;
    for (var i = 0; i < n; ++i) {
        var c = classes[i];
        if (c.level == level) {
            levelClasses.push(c);
        }
    }
    return levelClasses;
}

function createClassElements(levelClasses, maxTextWidth, maxTextHeight) {
    var textDimensions = { width: maxTextWidth, height: maxTextHeight };
    var n = levelClasses.length;
    for (var i = 0; i < n; ++i) {
        var levelClass = levelClasses[i];
        var svg = document.getElementById("classInheritanceDiagram");
        var rectElement = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
        rectElement.setAttribute("id", levelClass.id + "_rect");
        var linkElement = document.createElementNS('http://www.w3.org/2000/svg', 'a');
        linkElement.setAttribute("href", levelClass.link);
        var textElement = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        linkElement.appendChild(textElement);
        textElement.setAttribute("id", levelClass.id + "_text");
        textElement.innerHTML = levelClass.name;
        svg.appendChild(rectElement);
        svg.appendChild(linkElement);
        var bb = textElement.getBBox();
        var textWidth = bb.width;
        var textHeight = bb.height;
        levelClass.textWidth = textWidth;
        levelClass.textHeight = textHeight;
        if (textWidth > textDimensions.width) {
            textDimensions.width = textWidth;
        }
        if (textHeight > textDimensions.height) {
            textDimensions.height = textHeight;
        }
    }
    return textDimensions;
}

function drawDiagram(classes) {
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
    var allLevelClasses = [];
    while (cont) {
        var levelClasses = pick(level, classes);
        var n = levelClasses.length;
        cont = n > 0;
        if (cont) {
            var textDimensions = createClassElements(levelClasses, maxTextWidth, maxTextHeight);
            if (textDimensions.width > maxTextWidth) {
                maxTextWidth = textDimensions.width;
            }
            if (textDimensions.height > maxTextHeight) {
                maxTextHeight = textDimensions.height;
            }
            allLevelClasses.push(levelClasses);
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
        var levelClasses = allLevelClasses[level];
        var n = levelClasses.length;
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
    var svg = document.getElementById("classInheritanceDiagram");
    svg.setAttribute("width", totalWidth.toString());
    svg.setAttribute("height", totalHeight.toString());
    var prevRectY = 0;
    var prevRectX = 0;
    var prevHandleX2 = -1;
    var prevHandleY2 = -1;
    var prevY = 0;
    for (level = 0; level < maxLevel; ++level) {
        var direction = levelDirection[level];
        var levelClasses = allLevelClasses[level];
        var n = levelClasses.length;
        var rectY = prevY;
        prevY += levelHeight[level];
        var rectX = (totalWidth / n - rectWidth) / 2;
        var minHandleX = Number.MAX_SAFE_INTEGER;
        var maxHandleX = 0;
        var handleY = 0;
        for (var i = 0; i < n; ++i) {
            var levelClass = levelClasses[i];
            var textWidth = levelClass.textWidth;
            var textHeight = levelClass.textHeight;
            if (direction == horizontalDirection) {
                rectX = (totalWidth / n - rectWidth) / 2 + i * (rectWidth + rectXSpace);
            }
            else if (direction == verticalDirection) {
                rectX = prevRectX + (rectWidth + rectXSpace);
                rectY = prevRectY + horizontalRectYSpace + i * (rectHeight + verticalRectYSpace);
            }
            var textX = rectX + (rectWidth - textWidth) / 2;
            var textY = (rectY + rectHeight - yspace / 2) - (rectHeight - textHeight) / 2;
            var rectElement = document.getElementById(levelClass.id + "_rect");
            rectElement.setAttribute("x", rectX.toString());
            rectElement.setAttribute("y", rectY.toString());
            rectElement.setAttribute("width", rectWidth.toString());
            rectElement.setAttribute("height", rectHeight.toString());
            var fillColor = "white";
            if (levelClass.subject) {
                fillColor = "floralWhite";
            }
            rectElement.setAttribute("fill", fillColor);
            rectElement.setAttribute("stroke", "black");
            var textElement = document.getElementById(levelClass.id + "_text");
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
            } else if (level == maxLevel - 1 && levelClass.hasDerivedClasses) {
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

function drawClassInheritanceDiagram() {
    var classes = [
        { name: "cmajor::ast::Node", id: "class_0", level: 0, subject: false, hasDerivedClasses: true, link: "class_Node_66FC1D6AD8F487E453CE6E17163479C7C2DAC063.html" },
        { name: "cmajor::ast::LiteralNode", id: "class_1", level: 1, subject: true, hasDerivedClasses: true, link: "class_LiteralNode_33955D862408948682E1F5E18AC7C76B2C911BCC.html" },
        { name: "cmajor::ast::ArrayLiteralNode", id: "class_2", level: 2, subject: false, hasDerivedClasses: false, link: "class_ArrayLiteralNode_626CB13098008A2C46B156727CB76340707DC08B.html" },
        { name: "cmajor::ast::BooleanLiteralNode", id: "class_3", level: 2, subject: false, hasDerivedClasses: false, link: "class_BooleanLiteralNode_A7A23741EC6F82E852BC48AC11CF36BF843F6234.html" },
        { name: "cmajor::ast::ByteLiteralNode", id: "class_4", level: 2, subject: false, hasDerivedClasses: false, link: "class_ByteLiteralNode_B51A96304E3078151A6B48E5929D124117F47E33.html" },
        { name: "cmajor::ast::CharLiteralNode", id: "class_5", level: 2, subject: false, hasDerivedClasses: false, link: "class_CharLiteralNode_853FD806B72D3BA619C7A151EBD108B75C80A23A.html" },
        { name: "cmajor::ast::DoubleLiteralNode", id: "class_6", level: 2, subject: false, hasDerivedClasses: false, link: "class_DoubleLiteralNode_089F27B92140ED94881DE9924E1EADAA41E85849.html" },
        { name: "cmajor::ast::FloatLiteralNode", id: "class_7", level: 2, subject: false, hasDerivedClasses: false, link: "class_FloatLiteralNode_947EDC5869A9988296DAC286558871AA4A279B87.html" },
        { name: "cmajor::ast::IntLiteralNode", id: "class_8", level: 2, subject: false, hasDerivedClasses: false, link: "class_IntLiteralNode_EDEF90AB1D70920360AD54854C5AC511D2C66DC6.html" },
        { name: "cmajor::ast::LongLiteralNode", id: "class_9", level: 2, subject: false, hasDerivedClasses: false, link: "class_LongLiteralNode_A3EEBBE9EAD6AB77DEB64EF9F16E5A5EE6E7FC9C.html" },
        { name: "cmajor::ast::NullLiteralNode", id: "class_10", level: 2, subject: false, hasDerivedClasses: false, link: "class_NullLiteralNode_1205816D85E099712EC57C2AE8D2DBB78A311B4A.html" },
        { name: "cmajor::ast::SByteLiteralNode", id: "class_11", level: 2, subject: false, hasDerivedClasses: false, link: "class_SByteLiteralNode_EFFA2127AAF162622EFC2733821C513239CE30DE.html" },
        { name: "cmajor::ast::ShortLiteralNode", id: "class_12", level: 2, subject: false, hasDerivedClasses: false, link: "class_ShortLiteralNode_1238D6BC3751DF20B4897F0A9A21939AF1E433DE.html" },
        { name: "cmajor::ast::StringLiteralNode", id: "class_13", level: 2, subject: false, hasDerivedClasses: false, link: "class_StringLiteralNode_F90E5558C716D1481CDEA2156445EC31DFACF8B2.html" },
        { name: "cmajor::ast::StructuredLiteralNode", id: "class_14", level: 2, subject: false, hasDerivedClasses: false, link: "class_StructuredLiteralNode_BE7BAA9863FAEBD5A234C6BE0BE045742F51DF51.html" },
        { name: "cmajor::ast::UCharLiteralNode", id: "class_15", level: 2, subject: false, hasDerivedClasses: false, link: "class_UCharLiteralNode_6C86FCBA25F69A5EA1296F50E95091428E38ED63.html" },
        { name: "cmajor::ast::UIntLiteralNode", id: "class_16", level: 2, subject: false, hasDerivedClasses: false, link: "class_UIntLiteralNode_FABC012ACF8B6502A2BC20A78444294F55BADF5B.html" },
        { name: "cmajor::ast::ULongLiteralNode", id: "class_17", level: 2, subject: false, hasDerivedClasses: false, link: "class_ULongLiteralNode_5BAD9319E44930DC9F963975B985B499E15559ED.html" },
        { name: "cmajor::ast::UShortLiteralNode", id: "class_18", level: 2, subject: false, hasDerivedClasses: false, link: "class_UShortLiteralNode_AF3678A602FB1A8DFD3EB31518702B93C39FDA6C.html" },
        { name: "cmajor::ast::UStringLiteralNode", id: "class_19", level: 2, subject: false, hasDerivedClasses: false, link: "class_UStringLiteralNode_D018E65FC5BE355FFF8B3CA83821278F0287D803.html" },
        { name: "cmajor::ast::UuidLiteralNode", id: "class_20", level: 2, subject: false, hasDerivedClasses: false, link: "class_UuidLiteralNode_DE3DFDBE49C41D2302DD5EF6B4675F9E98C4579B.html" },
        { name: "cmajor::ast::WCharLiteralNode", id: "class_21", level: 2, subject: false, hasDerivedClasses: false, link: "class_WCharLiteralNode_4855E83EEA496E6755671CEEC5314298FD639517.html" },
        { name: "cmajor::ast::WStringLiteralNode", id: "class_22", level: 2, subject: false, hasDerivedClasses: false, link: "class_WStringLiteralNode_71D68ED0713B9F5C6DF3EF9D4057DE9D06FCD7C4.html" }];
    drawDiagram(classes);
}

