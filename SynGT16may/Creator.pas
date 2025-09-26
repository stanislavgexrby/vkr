unit Creator;
interface
uses
  Classes, DrawObject, RegularExpression, Semantic, Ward;
const
  cVerticalSpace=cNS_Radius+7;

Procedure CreateDrawObjects(var AList:TDrawObjectList{InOut};ARE_Tree:TRE_Tree{In};Form:TComponent);

implementation
uses Arrow, Child;

Procedure CreateDrawObjects(var AList:TDrawObjectList{InOut};ARE_Tree:TRE_Tree{In};Form:TComponent);
var
  Height:integer;
  Semantic:TSemanticIDList;
  PrevDO,LastDO:TDrawObject;
  FirstDO:TDrawObjectFirst;
begin
  Semantic:=nil;
  AList.Clear;
  FirstDO:=TDrawObjectFirst.Create(Form);
  FirstDO.Place;
  AList.add(FirstDO);
  PrevDO:=ARE_Tree.DrawObjectsToRight(AList, Semantic, FirstDO, cwFORWARD, Height);
  LastDO:=TDrawObjectLast.Create(Form);
  if Semantic=nil then
    LastDO.InArrow:=TArrow.make(cwFORWARD, PrevDO)
  else
    LastDO.InArrow:=TSemanticArrow.make(cwFORWARD, PrevDO, Semantic);
  LastDO.x:=PrevDO.EndX+LastDO.InArrow.getLength;
  LastDO.y:=PrevDO.y;
  AList.add(LastDO);
  AList.Width:=LastDO.endX+cHorizontalSkipFromBorder;
//  if Height<cNS_Radius then Height:=cNS_Radius;
  AList.Height:=Height+FirstDO.y+cVerticalSkipFromBorder+cNS_Radius;
end;
end.
