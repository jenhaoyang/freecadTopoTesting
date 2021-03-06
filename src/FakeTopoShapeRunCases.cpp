/*********************************************************************************
*   Copyright (C) 2016 Wolfgang E. Sanyer (ezzieyguywuf@gmail.com)               *
*                                                                                *
*   This program is free software: you can redistribute it and/or modify         *
*   it under the terms of the GNU General Public License as published by         *
*   the Free Software Foundation, either version 3 of the License, or            *
*   (at your option) any later version.                                          *
*                                                                                *
*   This program is distributed in the hope that it will be useful,              *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of               *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
*   GNU General Public License for more details.                                 *
*                                                                                *
*   You should have received a copy of the GNU General Public License            *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.        *
******************************************************************************** */
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepTools.hxx>

#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <Standard_Failure.hxx>
#include <Geom_Plane.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_Line.hxx>
#include <Precision.hxx>

#include "FakeTopoShape.cpp"

//TopoShape ChangeCylinderHeight(const double height, const TopoShape Shape);

//TopoShape ChangeCylinderHeight(const double height, const TopoShape Shape){
    //TopoShape outShape = Shape;
//}

TopoShape DuplicateCylinderFilletBug(){
    TopoDS_Shape Box      = BRepPrimAPI_MakeBox(10., 10., 10.);
    TopoDS_Shape Cylinder = BRepPrimAPI_MakeCylinder(2., 10.);

    // in FreeCAD, every operation first creates a blank TopoShape and then adds the
    // TopoDS_Shape to it
    TopoShape BoxShape, CylShape, FusedShape;

    // The current FreeCAD source will directly set TopoShape._Shape from all kinds of
    // places in the code. I have added a TopoShape::setShape method so that we can have
    // some control over this. Note that this method is overloaded a few times
    BoxShape.setShape(Box, "Primitive Box");
    CylShape.setShape(Cylinder, "Primitive Cylinder");

    // As of now, I don't see the TopoShape fuse being used. rather, the fuse is done
    // outside and then stored to the TopoShape
    BRepAlgoAPI_Fuse mkFuse(Box, Cylinder);

    // in my current implementation, I send the FIRST TopoShape from the Fuse operation to
    // the setShape, and copy it's _TopoNamer
    FusedShape.setShape(BoxShape, mkFuse);

    // Now let's select an edge, then fillet it
    TopoDS_Shape BaseShape = mkFuse.Shape();

    // Finaly, 'Select' the edge
    std::string selectionLabel = FusedShape.selectEdge(3);
    std::cout << "Selected Edge Node = " << selectionLabel << std::endl;

    // Write out the current-state of things
    //FusedShape.OCCDeepDump();
    FusedShape.WriteTNamingNode("0:1:1", "01_Selected", true);
    FusedShape.WriteTNamingNode("0:2", "02_BaseBox", false);
    FusedShape.WriteTNamingNode("0:3", "03_BaseCyl", false);
    FusedShape.WriteTNamingNode("0:4", "04_FusedShape", false);

    // Fillet the selected edge
    TopoDS_Edge RecoveredEdge = FusedShape.getSelectedEdge(selectionLabel);
    BRepFilletAPI_MakeFillet mkFillet(FusedShape.getShape());
    mkFillet.Add(2., 2., RecoveredEdge);
    mkFillet.Build();

    // Record the Fillet Operation
    TopoShape FilletedShape;// = mkFillet.Shape();
    FilletedShape.setShape(FusedShape, mkFillet);

    // write out the latest node
    FusedShape.WriteTNamingNode("0:5", "05_FilletedShape", false);
    
    //--------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------
    //                  FUSE DONE, ABOUT TO RESIZE THE CYLINDER
    //--------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------

    // resize the cylinder. I believe FreeCAD just creates a whole new cylinder
    TopoDS_Shape Cylinder2 = BRepPrimAPI_MakeCylinder(2., 15.);
    TopoShape NewCylShape;
    NewCylShape.setShape(Cylinder2);

    // Now the FreeCAD algorithms figure out that Fuse and Fillet need to be re-run
    // First, re-do Fuse
    TopoShape ReFusedShape;
    BRepAlgoAPI_Fuse mkFuse2(BoxShape.getShape(), NewCylShape.getShape());
    ReFusedShape.setShape(BoxShape, mkFuse2);

    BRepFilletAPI_MakeFillet mkFillet2(ReFusedShape.getShape());
    // grab the selected edge from our tree to re-fillet
    TopoDS_Edge RecoveredEdge2 = ReFusedShape.getSelectedEdge(selectionLabel);

    // now re-do fillet
    mkFillet2.Add(2., 2., RecoveredEdge);
    mkFillet2.Build();

    // Record the Fillet Operation
    TopoShape ReFilletedShape;// = mkFillet2.Shape();
    ReFilletedShape.setShape(ReFusedShape, mkFillet2);

    // Write out some more shapes
    FusedShape.WriteTNamingNode("0:6", "06_PartOfFusionMaybe", false);
    FusedShape.WriteTNamingNode("0:7", "07_ReFusion", false);
    FusedShape.WriteTNamingNode("0:8", "08_ReFillet", false);

    return FusedShape;
}

TopoShape SimpleBoxWithNaming(){
    // We'll do this like freecad, first create TopoDS_Shape
    const TopoDS_Shape& Box = BRepPrimAPI_MakeBox(10., 10., 10.);
    // create Base Shape that gets passed to FilletFeature
    TopoShape BaseShape(Box);
    //std::clog << "Dumping deep TDF tree\n";
    //std::clog << BaseShape.DeepDeepDumpTopoHistory();

    // create blank TopoShape in FilletFeature and add Base Shape
    TopoShape BoxShape;
    BoxShape.addShape(Box);

    // Finaly, 'Select' the edge
    std::cout << "Trying to select the first time" << std::endl;
    std::string selectionLabel = BoxShape.selectEdge(3);
    //std::cout << "Trying to select a second time" << std::endl;
    //std::string selectionLabel2 = BoxShape.selectEdge(3);
    //std::clog << "Dumping tree" << std::endl;
    //std::clog << BoxShape.DumpTopoHistory();
    //std::clog << "|____________________|" << std::endl;
    
    // And fillet the box
    std::clog << "Running makeTopoShape" << std::endl;
    BoxShape.makeTopoShapeFillet(2., 2., selectionLabel);

    //std::clog << "-------------------------" << std::endl;
    std::clog << "------------ REBUILDING ----------" << std::endl;
    //std::clog << "-------------------------" << std::endl;

    // make box taller
    const TopoDS_Shape& Box2 = BRepPrimAPI_MakeBox(10., 20., 20.);
    BoxShape.modifyShape("0:2", Box2);
    std::clog << BoxShape.DumpTopoHistory();


    // try re-building the box
    TopoDS_Edge recoveredEdge = TopoDS::Edge(BoxShape.getSelectedEdge(selectionLabel));
    //TopoDS_Shape recoveredBase = BoxShape.getSelectedBaseShape(selectionLabel);
    //TopoDS_Shape recoveredBase = BoxShape.getNodeShape("0:4");
    TopoDS_Shape recoveredBase = BoxShape.getTipShape();

    //if (recoveredBase.IsNull()){
        //std::cout << "BaseShape is NULL....\n";
    //}
    //else{
        //std::cout << "BaseShape is not NULL!!!!\n";
    //}

    //if (recoveredEdge.IsNull()){
        //std::cout << "recoveredEdge is Nul..." << std::endl;
    //}
    //else{
        //std::cout << "recoveredEdge is NOT NULL!!!!" << std::endl;
    //}

    TopTools_IndexedMapOfShape edges;
    TopExp::MapShapes(recoveredBase, TopAbs_EDGE, edges);
    if (edges.Contains(recoveredEdge)){
        std::cout << "Yes the Edge is in the box...\n";
    }
    else{
        std::cout << "No, the Edge is not in the Box...\n";
    }

    //BoxShape.makeTopoShapeFillet(2., 2., selectionLabel);

    return BoxShape;
}

void TestMkFillet(){
    TopoDS_Shape Box = BRepPrimAPI_MakeBox(10., 10., 10.);
    BRepFilletAPI_MakeFillet mkFillet(Box);

    TopTools_IndexedMapOfShape edges;
    TopExp::MapShapes(Box, TopAbs_EDGE, edges);
    TopoDS_Edge edge = TopoDS::Edge(edges.FindKey(3));

    mkFillet.Add(1., 1., edge);
    mkFillet.Build();

    TopoDS_Shape result = mkFillet.Shape();


    TopTools_IndexedMapOfShape faces;
    TopExp::MapShapes(Box, TopAbs_FACE, faces);
    TopoDS_Face face = TopoDS::Face(faces.FindKey(3));

    TopTools_ListOfShape modified = mkFillet.Modified(face);
    TopTools_ListIteratorOfListOfShape modIt;
    for (int i=1; modIt.More(); modIt.Next(), i++){
        TopoDS_Face curFace = TopoDS::Face(modIt.Value());
        TopoNamingHelper::WriteShape(curFace, "00_02_ModifiedFace", i);
    }

    TopoNamingHelper::WriteShape(result, "00_00_FilletResult");
    TopoNamingHelper::WriteShape(face, "00_01_BaseFace_3");
}
// Main func
int main(){
    try{
        //TopoShape res = DuplicateCylinderFilletBug();
        //std::clog << "--------------------------------------------------" << std::endl;
        //TopoShape res = SimpleBoxWithNaming();
        //res.DumpTopoHistory();
        TestMkFillet();
    }
    catch(Standard_Failure failure){
        failure.Print(std::clog);
        std::clog << std::endl;
        failure.Reraise();
    }
    return 0;
}
