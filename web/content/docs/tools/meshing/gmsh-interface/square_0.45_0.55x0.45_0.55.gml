<?xml version="1.0" encoding="ISO-8859-1"?>
<?xml-stylesheet type="text/xsl" href="OpenGeoSysGLI.xsl"?>

<OpenGeoSysGLI xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:ogs="http://www.opengeosys.org">
    <name>square_0.45_0.55x0.45_0.55_geometry</name>
    <points>
        <point id="0" x="0.45" y="0.45" z="0"/>
        <point id="1" x="0.45" y="0.55" z="0"/>
        <point id="2" x="0.55" y="0.45" z="0"/>
        <point id="3" x="0.55" y="0.55" z="0"/>
    </points>

    <polylines>
        <polyline id="4" name="boundary">
            <pnt>0</pnt>
            <pnt>1</pnt>
            <pnt>3</pnt>
            <pnt>2</pnt>
            <pnt>0</pnt>
        </polyline>
    </polylines>
</OpenGeoSysGLI>
