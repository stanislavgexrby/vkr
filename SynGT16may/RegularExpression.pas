unit RegularExpression;

interface
uses
    CharProducer, comctrls, Windows,Classes, DrawObject, Graphics, Semantic;
Const
    cmNotMarked=0;
    cmWasOpened=1;
    cmWasNotOpened=2;
    cmInProgress=3;

    cmMarkedEmpty = 1;
    cmMarkedNotEmpty = 2;

    lenNonTerminals=Length('NONTERMINALS');
    lenEnvironment=Length('ENVIRONMENT');
    OperationTerminal=0;
    cgTerminalList=OperationTerminal;
    OperationSemantic=1;
    cgSemanticList=OperationSemantic;
    OperationNonTerminal=2;
    cgNonTerminalList=OperationNonTerminal;
    OperationMacro=3;
    cgMacroList=OperationMacro;
    OperationOr=4;
    OperationAnd=5;
    OperationIteration=6;
type

    TSelMas = array of integer;      // Indexes of selected objects. For Substitute

    TTerminalList = class (TComponent)
    private
        m_Items:TStrings;
    public
        constructor make(Owner:TComponent;Items:TStrings);
        procedure fillNew;

        Procedure SaveNames;
        Procedure LoadNames;

        function getLengthFromID(index:integer):integer;
        function getString(index:integer):string;
        function add(const s:string):integer;virtual;
        function find(const s:string):integer;
        function remove(id: integer): integer;
    end;

    TSemanticList = class(TTerminalList)
        procedure fillNew;
        function add(const s:string):integer;override;
    end;

    TMacroList = class (TSemanticList)
    public
        procedure OpenAllMacro;
        procedure SetAllNotMarked;
        procedure CloseAllDefinition;

        Procedure SaveObjects;
        Procedure LoadObjects;

        procedure fillNew;
        function add(const s:string):integer;override;
        function getValue(index:integer):string;
        procedure setValueFromRoot(index:integer);
        procedure setStringFromRoot(index:integer);
        procedure setAllValuesFromRoot;
        procedure setValue(index:integer;const pValue:string);
        property StringValues[Index: Integer]: string read getValue write setValue;
    end;

    TNonTerminalList = class (TMacroList)
    private
        procedure unmarkAllNonterminals;
        procedure markEmptyNonterminals;
    public
        procedure regularizeGrammar();
        //This Procedure delete all left and right recursion.
        //And regularize Grammar.

        procedure fillNew;
    end;

    TRE_Tree = class(TComponent) //Owner is TGrammar
    private
        m_Op:byte;
        m_Mark:byte;
        DrawObj: Integer;     // Pointed to DrawObject (if presented). For Substitute
    protected
        function getPriority:integer;virtual;
        function DrawObjectsCanDown(AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{Out};AFromDO:TDrawObjectPoint{In Out};
             Ward:integer{In}; var Height:integer{Out}
             ):TDrawObject{LastDrawObject Out};virtual;
        function DrawObjectsFromPoint(AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{Out};var AFromDO:TDrawObjectPoint{In Out};
             Ward:integer{In}; var Height:integer{Out}
             ):TDrawObject{LastDrawObject Out};virtual;
        function DrawObjectsToBothLastPoints(AList:TDrawObjectList{In Changed};
             ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
             var Height:integer{Out};var ALastDownDO:TDrawObjectPoint {Out}
             ):TDrawObjectPoint{LastDrawObject Out};virtual;
        function DrawObjectsToPoint(AList:TDrawObjectList{In Changed};
             ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
             var Height:integer{Out}
             ):TDrawObjectPoint{LastDrawObject Out};virtual;
        function DrawObjectsToDown(AList:TDrawObjectList{In Changed};
             ALeftDO:TDrawObjectPoint{In};Ward:integer{In}; var Height:integer{Out}
             ):TDrawObjectPoint{LastDrawObject Out};virtual;
        function DrawObjectsToDownWithBothDownPoints(
             AList:TDrawObjectList{In Changed};var ALeftDO:TDrawObjectPoint{In};
             Ward:integer{In};var Height:integer{Out};
             var ALastDownDO:TDrawObjectPoint
             ):TDrawObjectPoint{LastDrawObject Out};virtual;
        procedure tryToSetEmptyMark;virtual;abstract;
    public
        procedure substituteAllEmpty;virtual;abstract;

        procedure unmarkAll;virtual;
        //set mark for all tree to cmNotMarked
        function getEmptyMark:byte;
        //set if possible mark for all tree to

        function AllMacroWasOpened:boolean;virtual;
        function AllDefinitionWasClosed:boolean;virtual;

        Procedure Save;virtual;abstract;

        function toString(SelMas: TSelMas; Reverse: boolean):string;virtual;abstract;  // Parameter for substitute
        function DrawObjectsToRight (AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{InOut};AFromDO:TDrawObject{In};
             Ward:integer{In}; var Height:integer{Out}
             ):TDrawObject{LastDrawObject Out};virtual;
         function Copy:TRE_Tree;virtual;abstract;

         function Left: TRE_Tree;virtual;              // left child
         function Right: TRE_Tree;virtual;             // right child
         property DrawObjId:integer read DrawObj;
    published
        property Operation:byte read m_Op write m_Op;
        property Priority:integer read getPriority;
    end;

    TGrammar = class;

    TNTListItem = Class(TComponent) //Owner is TGrammar
    private
        m_Value : string;
        m_Root : TRE_Tree;
        m_Mark : integer;
        m_Draw : TDrawObjectList;
        m_LastX :integer;
        m_LastY :integer;
        function getHeight:integer;
        function getWidth:integer;
        procedure setHeight(H:integer);
        procedure setWidth(W:integer);
    public
        constructor Create(Owner:TComponent);override;
        Procedure Save;
        Procedure Load;

        Procedure SelectedMoveTo(AX,AY:integer);
        Procedure SelectedSetTo(AX,AY:integer);
        Procedure UnSelectAll;
        Procedure addExtendedPoint;
        Procedure ChangeSelectionInRect(ARect:TRect);
        function FindDO(x,y:integer):TDrawObject;
        Procedure ChangeSelection(ATarget:TDrawObject);
        Procedure SelectAllNotSelected(ATarget:TDrawObject);

        procedure Draw(Canvas:TCanvas);
        function CopyRE_Tree:TRE_Tree;
        procedure setValue(const pValue:string);
        procedure setValueFromRoot;
        procedure setStringFromRoot;
        procedure setRootFromValue;
        function setMarkAsOpenAllMacro:integer;

        property Value: string read m_Value;
        property Height:integer read getHeight write setHeight;
        property Width:integer read getWidth write setWidth;
        property Mark:integer read m_Mark write m_Mark;
        property Root : TRE_Tree read m_Root write m_Root;
        property DrawObj: TDrawObjectList read m_Draw;       // Need for substitute
    end;

    TGrammar = class(TComponent)//Owner is TChildForm
    private
      m_TerminalList : TTerminalList;
      m_SemanticList : TSemanticList;
      m_NonTerminalList : TNonTerminalList;
      m_MacroList : TMacroList;

      procedure AddToDictionary(DictionaryID:integer;ACharProducer:TCharProducer);
    public
        constructor Make(Owner:TComponent;TerminalList,
           SemanticList,NonTerminalList,MacroList:TStrings);

        procedure regularizeGrammar();
        //This Procedure delete all left and right recursion.

        procedure ClearAllDictionaries;
        procedure fillNew;
        procedure setValuesFromRoot;
        procedure Load(const FileName:string);
        procedure ImportFromRichEdit( RichEdit: TRichEdit );
        procedure ImportFromSyntax(const FileName:string);
        procedure ImportFromGEdit(const FileName:string);
        procedure Save(const FileName:string);

        function Find(ListID:integer;const Name:string):integer;
        function FindName(const Name:string):boolean;
        function FindTerminalName(const Name:string):integer;
        function FindNonTerminalName(const Name:string):integer;
        function FindSemanticName(const Name:string):integer;
        function FindMacroName(const Name:string):integer;

        function MacroObject(id:integer):TNTListItem;
        function NonTerminalObject(id:integer):TNTListItem;
        function AddName(ListID:integer;const Name:string):integer;
        function AddTerminalName(const Name:string):integer;
        function AddNonTerminalName(const Name:string):integer;
        function AddMacroName(const Name:string):integer;
        function AddSemanticName(const Name:string):integer;

        function RemoveNonterminal(id: integer): integer;

        function getFreeNonTerminalName:string;
        function getFreeMacroName:string;
        function getFreeSemanticName:string;

        function GetName(ListID, NameID:integer):string;
        function GetTerminalName(NameID:integer):string;
        function GetNonTerminalName(NameID:integer):string;
        function GetSemanticName(NameID:integer):string;
        function GetMacroName(NameID:integer):string;

        function getTerminals: TStrings;
        function getSemantics: TStrings;
        function getNonTerminals: TStrings;
        function getMacros: TStrings;

        procedure OpenAllMacro;
        procedure CloseAllDefinition;

        property MacroObjects[Index: Integer]: TNTListItem read MacroObject;
        property NonTerminalObjects[Index: Integer]: TNTListItem read NonTerminalObject;

        property TerminalList : TStrings read getTerminals;
        property SemanticList : TStrings read getSemantics;
        property NonTerminalList : TStrings read getNonTerminals;
        property MacroList : TStrings read getMacros;
    end;

    TRE_Leaf = class(TRE_Tree)
    private
        m_ID:integer;
        function getNameFromID:string;
        procedure setNameFromID(const name:string);
    protected
    public
        procedure substituteAllEmpty;override;
        Procedure Save;override;

        function toString(SelMas: TSelMas; Reverse: boolean):string;override;   // Parameter for substitute
        function Left: TRE_Tree; override;              // left child
        function Right: TRE_Tree; override;             // right child
    published
        property ID: integer read m_ID write m_ID;
        property NameID: string read getNameFromID write setNameFromID;
    end;

    TRE_Terminal= class (TRE_Leaf)
    protected
        procedure tryToSetEmptyMark;override;
    public
        function toString(SelMas: TSelMas; Reverse: boolean):string;override;    // Parameter for substitute
        function Copy:TRE_Tree;override;
        constructor Create(AOwner:TComponent);override;
        constructor makeFromID(AOwner:TComponent; ID:integer);

        function DrawObjectsToRight (AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{InOut};AFromDO:TDrawObject{In};
             Ward:integer{In}; var Height:integer{Out}
            ):TDrawObject{LastDrawObject Out};override;
        function Left: TRE_Tree; override;           // left child
        function Right: TRE_Tree; override;            // right child
    end;
//    TRE_NullTerminal= class (TRE_Terminal)
//    end;

    TRE_Semantic= class (TRE_Leaf)
    protected
        procedure tryToSetEmptyMark;override;
    public
        function Copy:TRE_Tree;override;
        constructor Create(AOwner:TComponent);override;
        constructor makeFromID(AOwner:TComponent; ID:integer);

        function DrawObjectsToRight (AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{InOut};AFromDO:TDrawObject{In};
             Ward:integer{In}; var Height:integer{Out}
            ):TDrawObject{LastDrawObject Out};override;
    end;

    TRE_Macro = class (TRE_Leaf)
    protected
        function DrawObjectsCanDown(AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{Out};AFromDO:TDrawObjectPoint{In Out};
             Ward:integer{In}; var Height:integer{Out}
             ):TDrawObject{LastDrawObject Out};override;
        function DrawObjectsFromPoint(AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{Out};var AFromDO:TDrawObjectPoint{In Out};
             Ward:integer{In}; var Height:integer{Out}
             ):TDrawObject{LastDrawObject Out};override;
        function DrawObjectsToBothLastPoints(AList:TDrawObjectList{In Changed};
             ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
             var Height:integer{Out};var ALastDownDO:TDrawObjectPoint {Out}
             ):TDrawObjectPoint{LastDrawObject Out};override;
        function DrawObjectsToPoint(AList:TDrawObjectList{In Changed};
             ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
             var Height:integer{Out}
             ):TDrawObjectPoint{LastDrawObject Out};override;
        function DrawObjectsToDown(AList:TDrawObjectList{In Changed};
            ALeftDO:TDrawObjectPoint{In};Ward:integer{In}; var Height:integer{Out}
            ):TDrawObjectPoint{LastDrawObject Out};override;
        function DrawObjectsToDownWithBothDownPoints(
             AList:TDrawObjectList{In Changed};var ALeftDO:TDrawObjectPoint{In};
             Ward:integer{In};var Height:integer{Out};
             var ALastDownDO:TDrawObjectPoint
             ):TDrawObjectPoint{LastDrawObject Out};override;
        procedure tryToSetEmptyMark;override;
    private
        m_Opened : boolean;
        function getRoot:TRE_Tree;virtual;
        function getListItem:TNTListItem;virtual;
        function getPriority:integer;override;
    public
        constructor Create(AOwner:TComponent);override;
        constructor makeFromID(AOwner:TComponent; ID:integer);
        constructor makeFromIDAndOpen(AOwner:TComponent; ID:integer;open:boolean);

        function AllMacroWasOpened:boolean;override;
        function AllDefinitionWasClosed:boolean;override;
        Procedure Save;override;

        function toString(SelMas: TSelMas; Reverse: boolean):string;override;    // Parameter for substitute, right elim
        function Copy:TRE_Tree;override;

        property Opened:boolean read m_Opened write m_Opened;
        property Root:TRE_Tree read GetRoot;
        property ListItem:TNTListItem read getListItem;

        function DrawObjectsToRight (AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{InOut};AFromDO:TDrawObject{In};
             Ward:integer{In}; var Height:integer{Out}
            ):TDrawObject{LastDrawObject Out};override;
    end;

    TRE_NonTerminal= class (TRE_Macro)
    private
        function getRoot:TRE_Tree;override;
        function getListItem:TNTListItem;override;
        procedure tryToSetEmptyMark;override;
    public
        constructor Create(AOwner:TComponent);override;
        constructor makeFromID(AOwner:TComponent; ID:integer);
        constructor makeFromIDAndOpen(AOwner:TComponent; ID:integer; open:boolean);

        function AllMacroWasOpened:boolean;override;
        function Copy:TRE_Tree;override;
    end;

    TRE_BinaryOperation = class(TRE_Tree)
    private
        m_First,m_Second:TRE_Tree;
        function getOperationChar:char;
    protected
        function DrawObjectsToPoint(AList:TDrawObjectList{In Changed};
             ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
             var Height:integer{Out}):TDrawObjectPoint{LastDrawObject Out};override;
        function DrawObjectsCanDown(AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{Out};AFromDO:TDrawObjectPoint{In Out};
             Ward:integer{In}; var Height:integer{Out}
             ):TDrawObject{LastDrawObject Out};override;
    public
        procedure substituteAllEmpty;override;
        procedure unmarkAll;override;
        Procedure Save;override;

        function AllMacroWasOpened:boolean;override;
        function AllDefinitionWasClosed:boolean;override;

        function toString(SelMas: TSelMas; Reverse: boolean):string;override;    // Parameter for substitute, right elim  

        function Left: TRE_Tree; override;              // left child
        function Right: TRE_Tree; override;             // right child
    published
        property OperationChar:char read getOperationChar;
        property FirstOperand:TRE_Tree read m_First write m_First;
        property SecondOperand:TRE_Tree read m_Second write m_Second;
    end;
    TRE_Or = class(TRE_BinaryOperation)
    protected
        function DrawObjectsToDown(AList:TDrawObjectList{In Changed};
            ALeftDO:TDrawObjectPoint{In};Ward:integer{In}; var Height:integer{Out}
            ):TDrawObjectPoint{LastDrawObject Out};override;
        function DrawObjectsToBothLastPoints(AList:TDrawObjectList{In Changed};
             ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
             var Height:integer{Out};var ALastDownDO:TDrawObjectPoint {Out}
             ):TDrawObjectPoint{LastDrawObject Out};override;
        function DrawObjectsToDownWithBothDownPoints(
             AList:TDrawObjectList{In Changed};var ALeftDO:TDrawObjectPoint{In};
             Ward:integer{In};var Height:integer{Out};
             var ALastDownDO:TDrawObjectPoint
             ):TDrawObjectPoint{LastDrawObject Out};override;
        function DrawObjectsFromPoint(AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{Out};var AFromDO:TDrawObjectPoint{In};
             Ward:integer{In}; var Height:integer{Out}
             ):TDrawObject{LastDrawObject Out};override;
        procedure tryToSetEmptyMark;override;
    public
        function Copy:TRE_Tree;override;
        constructor make(AOwner:TComponent; first,Second:TRE_Tree);
    end;
    TRE_And = class(TRE_BinaryOperation)
    protected
        function DrawObjectsCanDown(AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{Out};AFromDO:TDrawObjectPoint{In Out};
             Ward:integer{In}; var Height:integer{Out}
             ):TDrawObject{LastDrawObject Out};override;
        function DrawObjectsFromPoint(AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{Out};var AFromDO:TDrawObjectPoint{In Out};
             Ward:integer{In}; var Height:integer{Out}
             ):TDrawObject{LastDrawObject Out};override;
        function DrawObjectsToBothLastPoints(AList:TDrawObjectList{In Changed};
             ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
             var Height:integer{Out};var ALastDownDO:TDrawObjectPoint {Out}
             ):TDrawObjectPoint{LastDrawObject Out};override;
        function DrawObjectsToPoint(AList:TDrawObjectList{In Changed};
             ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
             var Height:integer{Out}
             ):TDrawObjectPoint{LastDrawObject Out};override;
        function DrawObjectsToDown(AList:TDrawObjectList{In Changed};
            ALeftDO:TDrawObjectPoint{In};Ward:integer{In}; var Height:integer{Out}
            ):TDrawObjectPoint{LastDrawObject Out};override;
        function DrawObjectsToDownWithBothDownPoints(
             AList:TDrawObjectList{In Changed};var ALeftDO:TDrawObjectPoint{In};
             Ward:integer{In};var Height:integer{Out};
             var ALastDownDO:TDrawObjectPoint
             ):TDrawObjectPoint{LastDrawObject Out};override;
        procedure tryToSetEmptyMark;override;
    public
        function Copy:TRE_Tree;override;
        constructor make(AOwner:TComponent; first,Second:TRE_Tree);
        function DrawObjectsToRight (AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{InOut};AFromDO:TDrawObject{In};
             Ward:integer{In}; var Height:integer{Out}
            ):TDrawObject{LastDrawObject Out};override;
        // Parameter for substitute, right elim
        function toString(SelMas: TSelMas; Reverse: boolean):string;override;
    end;
    TRE_Iteration = class(TRE_BinaryOperation)
    protected
        function DrawObjectsFromPoint(AList:TDrawObjectList{In Changed};
             var ASemantic:TSemanticIDList{Out};var AFromDO:TDrawObjectPoint{In Out};
             Ward:integer{In}; var Height:integer{Out}
             ):TDrawObject{LastDrawObject Out};override;
        function DrawObjectsToDown(AList:TDrawObjectList{In Changed};
            ALeftDO:TDrawObjectPoint{In};Ward:integer{In}; var Height:integer{Out}
            ):TDrawObjectPoint{LastDrawObject Out};override;
        procedure tryToSetEmptyMark;override;
    public
        function Copy:TRE_Tree;override;
        constructor make(AOwner:TComponent; first,Second:TRE_Tree);
    end;
implementation
uses Parser,Parser2,Main,Child,Creator,sysutils,Dictionary,
   Dialogs,ErrorUnit,Ward,Arrow;

function makeArrowForwardAlways(AFromDO:TDrawObject;ward:integer;
              var ASemantic:TSemanticIDList):TArrow;
var
  curWard:integer;
begin
  if (AFromDO.NeedSpike) or (ward=cwFORWARD) then
    curWard:=ward
  else
    curWard:=cwNONE;
  if ASemantic=nil then
    result:=TArrow.make(curWard, AFromDO)
  else begin
    result:=TSemanticArrow.make(curWard, AFromDO, ASemantic);
    ASemantic:=nil;
  end;
end;
function makeArrowForwardNone(AFromDO:TDrawObject;ward:integer;
              var ASemantic:TSemanticIDList):TArrow;
var
  curWard:integer;
begin
  if (AFromDO.NeedSpike) and (ward=cwBACKWARD) then
    curWard:=cwBACKWARD
  else
    curWard:=cwNONE;
  if ASemantic=nil then
    result:=TArrow.make(curWard, AFromDO)
  else begin
    result:=TSemanticArrow.make(curWard, AFromDO, ASemantic);
    ASemantic:=nil;
  end;
end;
function CreatePointToRight(AList:TDrawObjectList;AFromDO:TDrawObject;
          ward:integer;Form:TComponent):TDrawObjectPoint;overload;
var
  PointDO:TDrawObjectPoint;
begin
  PointDO:=TDrawObjectPoint.Create(Form);
  PointDO.InArrow:=TArrow.make(Ward, AFromDO);
  PointDO.setPlaceToRight;
  AList.add(PointDO);
  result:=PointDO;
end;
function CreatePointToRight(AList:TDrawObjectList;AFromDO:TDrawObject; ward:integer;
      var ASemantic:TSemanticIDList; Form:TComponent):TDrawObjectPoint;overload;
var
  PointDO:TDrawObjectPoint;
begin
  PointDO:=TDrawObjectPoint.Create(Form);
  if ASemantic=nil then
    PointDO.InArrow:=TArrow.make(Ward, AFromDO)
  else begin
    PointDO.InArrow:=TSemanticArrow.make(Ward, AFromDO, ASemantic);
    ASemantic:=nil;
  end;
  PointDO.setPlaceToRight;
  AList.add(PointDO);
  result:=PointDO;
end;
function CreatePointToDown(AList:TDrawObjectList;AFromDO:TDrawObject; ward:integer;
      Y:integer; Form:TComponent):TDrawObjectPoint;
begin
  result:=TDrawObjectPoint.Create(Form);
  result.InArrow:=TArrow.make(Ward, AFromDO);
  result.x:=AFromDO.x;
  result.y:=Y;
  AList.add(result);
end;
function TRE_Tree.DrawObjectsToDown(AList:TDrawObjectList{In Changed};
    ALeftDO:TDrawObjectPoint{In};Ward:integer{In}; var Height:integer{Out}
    ):TDrawObjectPoint{LastDrawObject Out};
var
  LastDO:TDrawObject;
  Semantic:TSemanticIDList;
  curWard:integer;
begin
  Semantic:=nil;
  LastDO:=DrawObjectsCanDown(AList,Semantic,ALeftDO,Ward,Height);
  if(LastDO.NeedSpike)and (ward=cwBACKWARD)then
    curWard:=cwBACKWARD
  else
    curWard:=cwNONE;
  result:=CreatePointToRight(AList,LastDO,curWard,Semantic,Owner.Owner);
end;

function TRE_Terminal.DrawObjectsToRight (AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{InOut};AFromDO:TDrawObject{In};
     Ward:integer{In}; var Height:integer{Out}
    ):TDrawObject{LastDrawObject Out};
begin
  if m_ID<>0 then Height:=cNS_Radius else Height:=0;
  result:=TDrawObjectTerminal.make(Owner.Owner,ID);
  result.InArrow:=makeArrowForwardAlways(AFromDO,ward,ASemantic);
  result.setPlaceToRight;
  AList.add(result);
  DrawObj:= AList.Count-1;  // Assign 'Tree' and 'Draw' Objects. For Substitute
end;

function TRE_Semantic.DrawObjectsToRight (AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{InOut};AFromDO:TDrawObject{In};
     Ward:integer{In}; var Height:integer{Out}
    ):TDrawObject{LastDrawObject Out};
begin
  Height:=0;
  if ASemantic=nil then
    ASemantic:=TSemanticIDList.Create(TChildForm(Owner.Owner).SemanticTable);
  ASemantic.addToEnd(m_ID);
  result:=AFromDO;
end;
function TRE_Macro.DrawObjectsToRight(AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{InOut};AFromDO:TDrawObject{In};
     Ward:integer{In}; var Height:integer{Out}
    ):TDrawObject{LastDrawObject Out};
begin
  if opened then
    result:=Root.DrawObjectsToRight(AList,ASemantic,AFromDO,Ward,Height)
  else begin
    Height:=cNS_Radius;
    if m_Op=OperationMacro then
      result:=TDrawObjectMacro.make(Owner.Owner,m_ID)
    else
      result:=TDrawObjectNonTerminal.make(Owner.Owner,m_ID);
    result.InArrow:=makeArrowForwardAlways(AFromDO,ward,ASemantic);
    result.setPlaceToRight;
    ASemantic:=nil;
    AList.add(result);
    DrawObj:= AList.Count-1;            // Assign 'Tree' and 'Draw' Objects. For Substitute}
  end;
end;
function TRE_And.DrawObjectsToDown(AList:TDrawObjectList{In Changed};
    ALeftDO:TDrawObjectPoint{In};Ward:integer{In}; var Height:integer{Out}
    ):TDrawObjectPoint{LastDrawObject Out};
var
  Height1:integer;
  Semantic:TSemanticIDList;
  LastDO:TDrawObject;
begin
  if Ward=cwFORWARD then begin
    LastDO:=FirstOperand.DrawObjectsCanDown(AList,Semantic,ALeftDO,Ward,Height1);
    result:=SecondOperand.DrawObjectsToPoint(AList,Semantic,LastDO,Ward,Height);
  end else begin
    LastDO:=SecondOperand.DrawObjectsCanDown(AList,Semantic,ALeftDO,Ward,Height1);
    result:=FirstOperand.DrawObjectsToPoint(AList,Semantic,LastDO,Ward,Height);
  end;
  if Height < Height1 then Height:=Height1;
end;
function TRE_And.DrawObjectsCanDown(AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{Out};AFromDO:TDrawObjectPoint{In Out};
     Ward:integer{In}; var Height:integer{Out}
     ):TDrawObject{LastDrawObject Out};
var
  Height1:integer;
  LastDO:TDrawObject;
begin
  if Ward=cwFORWARD then begin
    LastDO:=FirstOperand.DrawObjectsCanDown(AList,ASemantic,AFromDO,Ward,Height1);
    result:=SecondOperand.DrawObjectsToRight(AList,ASemantic,LastDO,Ward,Height);
  end else begin
    LastDO:=SecondOperand.DrawObjectsCanDown(AList,ASemantic,AFromDO,Ward,Height1);
    result:=FirstOperand.DrawObjectsToRight(AList,ASemantic,LastDO,Ward,Height);
  end;
  if Height < Height1 then Height:=Height1;
end;

function TRE_And.DrawObjectsToRight (AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{InOut};AFromDO:TDrawObject{In};
     Ward:integer{In}; var Height:integer{Out}
    ):TDrawObject{LastDrawObject Out};
var
  Height1:integer;
begin
  if Ward=cwFORWARD then begin
    result:=FirstOperand.DrawObjectsToRight(AList,ASemantic,AFromDO,Ward,Height1);
    result:=SecondOperand.DrawObjectsToRight(AList,ASemantic,result,Ward,Height);
  end else begin
    result:=SecondOperand.DrawObjectsToRight(AList,ASemantic,AFromDO,Ward,Height);
    result:=FirstOperand.DrawObjectsToRight(AList,ASemantic,result,Ward,Height1);
  end;
  if Height < Height1 then Height:=Height1;
end;
function TRE_Tree.DrawObjectsToRight (AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{InOut};AFromDO:TDrawObject{In};
     Ward:integer{In}; var Height:integer{Out}
    ):TDrawObject{LastDrawObject Out};
var
  curWard:integer;
  LeftDO:TDrawObjectPoint;
begin
  if (ward=cwBACKWARD) and (AFromDO.NeedSpike) then
    curWard:=cwBACKWARD
  else
    curWard:=cwNONE;
  LeftDO:=CreatePointToRight(AList, AFromDO, curWard, ASemantic,Owner.Owner);
  result:=DrawObjectsCanDown(AList,ASemantic,LeftDO,Ward,Height);
end;

procedure TRE_Tree.unmarkAll;
begin
  m_Mark := cmNotMarked;
end;

procedure TRE_NonTerminal.tryToSetEmptyMark;
begin
  m_Mark := getRoot.m_Mark;
end;

procedure TRE_Semantic.tryToSetEmptyMark;
begin
  m_Mark := cmMarkedNotEmpty;
end;

procedure TRE_Macro.tryToSetEmptyMark;
begin
  m_Mark := cmMarkedNotEmpty;
end;

procedure TRE_And.tryToSetEmptyMark;
var
  leftMark:byte;
begin
  leftMark := FirstOperand.getEmptyMark;
  if not(leftMark = cmMarkedEmpty) then
  begin
    m_Mark:=leftMark;
  end
  else
  begin
    m_Mark:=SecondOperand.getEmptyMark;
  end;
end;

procedure TRE_Or.tryToSetEmptyMark;
var
  leftMark, rightMark:byte;
begin
  leftMark := FirstOperand.getEmptyMark;
  if (leftMark = cmMarkedEmpty) then
  begin
    m_Mark:=cmMarkedEmpty;
  end
  else
  begin
    rightMark := SecondOperand.getEmptyMark;
    if (rightMark = cmMarkedEmpty) then
    begin
      m_Mark:=cmMarkedEmpty;
    end
    else if (rightMark = cmMarkedNotEmpty) and (leftMark = cmMarkedNotEmpty) then
    begin
      m_Mark:=cmMarkedNotEmpty;
    end
    else
    begin
      m_Mark:=cmNotMarked;
    end;
  end;
end;

procedure TRE_Iteration.tryToSetEmptyMark;
begin
  m_Mark := FirstOperand.getEmptyMark;
end;

procedure TRE_Terminal.tryToSetEmptyMark;
begin
  if (NameID='') then
  begin
    m_Mark := cmMarkedEmpty;
  end
  else
  begin
    m_Mark := cmMarkedNotEmpty;
  end;
end;

function TRE_Tree.getEmptyMark:byte;
begin
  if (m_Mark=cmNotMarked) then
  begin
    tryToSetEmptyMark;
  end;
  result:=m_Mark;
end;

function TRE_Tree.DrawObjectsToPoint(AList:TDrawObjectList{In Changed};
     ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
     var Height:integer{Out}
     ):TDrawObjectPoint{LastDrawObject Out};
var
  LastDO:TDrawObject;
  curWard:integer;
begin
  LastDO:=DrawObjectsToRight(AList,ASemantic,AFromDO,Ward,Height);
  if (Ward=cwBACKWARD) and (LastDO.NeedSpike) then
    curWard:=cwBACKWARD
  else
    curWard:=cwNONE;
  result:=CreatePointToRight(AList, LastDO, curWard, ASemantic,Owner.Owner);
end;

function TRE_Tree.DrawObjectsToBothLastPoints(AList:TDrawObjectList{In Changed};
     ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
     var Height:integer{Out};var ALastDownDO:TDrawObjectPoint {Out}
     ):TDrawObjectPoint{LastDrawObject Out};
var
  LastDO:TDrawObject;
  curWard:integer;
begin
  LastDO:=DrawObjectsToRight(AList,ASemantic,AFromDO,Ward,Height);
  if (Ward=cwBACKWARD) and (LastDO.NeedSpike) then
    curWard:=cwBACKWARD
  else
    curWard:=cwNONE;
  ALastDownDO:=CreatePointToRight(AList, LastDO, curWard, ASemantic,Owner.Owner);
  result:=ALastDownDO;
end;

function TRE_Tree.DrawObjectsToDownWithBothDownPoints(
     AList:TDrawObjectList{In Changed};var ALeftDO:TDrawObjectPoint{In};
     Ward:integer{In};var Height:integer{Out};var ALastDownDO:TDrawObjectPoint
     ):TDrawObjectPoint{LastDrawObject Out};
var
  Semantic:TSemanticIDList;
begin
  Semantic:=nil;
  result:=DrawObjectsToBothLastPoints(Alist,Semantic,ALeftDO,Ward,Height,ALastDownDO);
end;
function TRE_Tree.DrawObjectsFromPoint(AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{Out};var AFromDO:TDrawObjectPoint{In Out};
     Ward:integer{In}; var Height:integer{Out}
     ):TDrawObject{LastDrawObject Out};
begin
  ASemantic:=nil;
  result:=DrawObjectsToRight(AList,ASemantic,AFromDO,Ward,Height);
end;
function TRE_Tree.DrawObjectsCanDown(AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{Out};AFromDO:TDrawObjectPoint{In Out};
     Ward:integer{In}; var Height:integer{Out}
     ):TDrawObject{LastDrawObject Out};
begin
  ASemantic:=nil;
  result:=DrawObjectsToRight(AList,ASemantic,AFromDO,Ward,Height);
end;

function TRE_And.DrawObjectsFromPoint(AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{Out};var AFromDO:TDrawObjectPoint{In Out};
     Ward:integer{In}; var Height:integer{Out}
     ):TDrawObject{LastDrawObject Out};
var
  LastDO:TDrawObject;
  Height1:integer;
begin
  ASemantic:=nil;
  if Ward=cwFORWARD then begin
    LastDO:=FirstOperand.DrawObjectsFromPoint(
              AList,ASemantic,AFromDO,Ward,Height1);
    LastDO:=SecondOperand.DrawObjectsToRight(
              AList,ASemantic,LastDO,Ward,Height);
  end else begin
    LastDO:=SecondOperand.DrawObjectsFromPoint(
              AList,ASemantic,AFromDO,Ward,Height1);
    LastDO:=FirstOperand.DrawObjectsToRight(
              AList,ASemantic,LastDO,Ward,Height);
  end;
  if Height < Height1 then Height:=Height1;
  result:=LastDO;
end;

function TRE_Or.DrawObjectsFromPoint(AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{Out};var AFromDO:TDrawObjectPoint{In Out};
     Ward:integer{In}; var Height:integer{Out}
     ):TDrawObject{LastDrawObject Out};
var
  LastDownDO:TDrawObjectPoint;
begin
  ASemantic:=nil;
  result:=DrawObjectsToDownWithBothDownPoints(AList,AFromDO,Ward,Height,LastDownDO);
end;
function TRE_And.DrawObjectsToBothLastPoints(AList:TDrawObjectList{In Changed};
     ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
     var Height:integer{Out};var ALastDownDO:TDrawObjectPoint {Out}
     ):TDrawObjectPoint{LastDrawObject Out};
var
  LastDO:TDrawObject;
  height1:integer;
begin
  if Ward=cwFORWARD then begin
    LastDO:=FirstOperand.DrawObjectsToRight(
                  AList,ASemantic,AFromDO,Ward,Height1);
    result:=SecondOperand.DrawObjectsToBothLastPoints(
                  AList,ASemantic,LastDO,Ward,Height,ALastDownDO);
  end else begin
    LastDO:=SecondOperand.DrawObjectsToRight(
                  AList,ASemantic,AFromDO,Ward,Height);
    result:=FirstOperand.DrawObjectsToBothLastPoints(
                  AList,ASemantic,LastDO,Ward,Height1,ALastDownDO);
  end;
  if Height < Height1 then Height:=Height1;
end;

function TRE_And.DrawObjectsToPoint(AList:TDrawObjectList{In Changed};
     ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
     var Height:integer{Out}
     ):TDrawObjectPoint{LastDrawObject Out};
var
  LastDO:TDrawObject;
  height1:integer;
begin
  if Ward=cwFORWARD then begin
    LastDO:=FirstOperand.DrawObjectsToRight(
                  AList,ASemantic,AFromDO,Ward,Height1);
    result:=SecondOperand.DrawObjectsToPoint(
                  AList,ASemantic,LastDO,Ward,Height);
  end else begin
    LastDO:=SecondOperand.DrawObjectsToRight(
                  AList,ASemantic,AFromDO,Ward,Height);
    result:=FirstOperand.DrawObjectsToPoint(
                  AList,ASemantic,LastDO,Ward,Height1);
  end;
  if Height < Height1 then Height:=Height1;
end;
function TRE_And.DrawObjectsToDownWithBothDownPoints(
     AList:TDrawObjectList{In Changed};var ALeftDO:TDrawObjectPoint{In};
     Ward:integer{In};var Height:integer{Out};var ALastDownDO:TDrawObjectPoint
     ):TDrawObjectPoint{LastDrawObject Out};
var
  Semantic:TSemanticIDList;
  LastDO:TDrawObject;
  height1:integer;
begin
  if Ward=cwFORWARD then begin
    LastDO:=FirstOperand.DrawObjectsFromPoint(
                  AList,Semantic,ALeftDO,Ward,Height1);
    result:=SecondOperand.DrawObjectsToBothLastPoints(
                  AList,Semantic,LastDO,Ward,Height,ALastDownDO);
  end else begin
    LastDO:=SecondOperand.DrawObjectsFromPoint(
                  AList,Semantic,ALeftDO,Ward,Height);
    result:=FirstOperand.DrawObjectsToBothLastPoints(
                  AList,Semantic,LastDO,Ward,Height1,ALastDownDO);
  end;
  if Height < Height1 then Height:=Height1;
end;
function TRE_Macro.DrawObjectsCanDown(AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{Out};AFromDO:TDrawObjectPoint{In Out};
     Ward:integer{In}; var Height:integer{Out}
     ):TDrawObject{LastDrawObject Out};
begin
  if opened then
    result:=Root.DrawObjectsCanDown(
                   AList,ASemantic,AFromDO,Ward,Height)
  else
    result:=inherited DrawObjectsCanDown(
                   AList,ASemantic,AFromDO,Ward,Height);
end;

function TRE_Macro.DrawObjectsFromPoint(AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{Out};var AFromDO:TDrawObjectPoint{In Out};
     Ward:integer{In}; var Height:integer{Out}
     ):TDrawObject{LastDrawObject Out};
begin
  if opened then
    result:=Root.DrawObjectsFromPoint(
                   AList,ASemantic,AFromDO,Ward,Height)
  else
    result:=inherited DrawObjectsFromPoint(
                   AList,ASemantic,AFromDO,Ward,Height)
end;
function TRE_Macro.DrawObjectsToBothLastPoints(AList:TDrawObjectList{In Changed};
     ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
     var Height:integer{Out};var ALastDownDO:TDrawObjectPoint {Out}
     ):TDrawObjectPoint{LastDrawObject Out};
begin
  if opened then
    result:=Root.DrawObjectsToBothLastPoints(
                   AList,ASemantic,AFromDO,Ward,Height,ALastDownDO)
  else
    result:=inherited DrawObjectsToBothLastPoints(
                   AList,ASemantic,AFromDO,Ward,Height,ALastDownDO)
end;
function TRE_Macro.DrawObjectsToPoint(AList:TDrawObjectList{In Changed};
     ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
     var Height:integer{Out}
     ):TDrawObjectPoint{LastDrawObject Out};
begin
  if opened then
    result:=Root.DrawObjectsToPoint(
                   AList,ASemantic,AFromDO,Ward,Height)
  else
    result:=inherited DrawObjectsToPoint(
                   AList,ASemantic,AFromDO,Ward,Height)
end;

function TRE_Macro.DrawObjectsToDownWithBothDownPoints(
     AList:TDrawObjectList{In Changed};var ALeftDO:TDrawObjectPoint{In};
     Ward:integer{In};var Height:integer{Out};var ALastDownDO:TDrawObjectPoint
     ):TDrawObjectPoint{LastDrawObject Out};
begin
  if opened then
    result:=Root.DrawObjectsToDownWithBothDownPoints(
                   AList,ALeftDO,Ward,Height,ALastDownDO)
  else
    result:=inherited DrawObjectsToDownWithBothDownPoints(
                   AList,ALeftDO,Ward,Height,ALastDownDO);
end;
function TRE_Macro.DrawObjectsToDown(AList:TDrawObjectList{In Changed};
    ALeftDO:TDrawObjectPoint{In};Ward:integer{In}; var Height:integer{Out}
    ):TDrawObjectPoint{LastDrawObject Out};
begin
  if opened then
    result:=Root.DrawObjectsToDown(
                   AList,ALeftDO,Ward,Height)
  else
    result:=inherited DrawObjectsToDown(
                   AList,ALeftDO,Ward,Height);
end;

function TRE_Or.DrawObjectsToBothLastPoints(AList:TDrawObjectList{In Changed};
     ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
     var Height:integer{Out};var ALastDownDO:TDrawObjectPoint {Out}
     ):TDrawObjectPoint{LastDrawObject Out};
var
  PointDO:TDrawObjectPoint;
  curWard:integer;
begin
  if (ward=cwBACKWARD) and (AFromDO.NeedSpike) then
    curWard:=cwBACKWARD
  else
    curWard:=cwNONE;
  PointDO:=CreatePointToRight(AList,AFromDO,curWard,Owner.Owner);
  result:=DrawObjectsToDownWithBothDownPoints(AList,PointDO,Ward,Height,ALastDownDO);
end;

function TRE_Or.DrawObjectsToDown(AList:TDrawObjectList{In Changed};
    ALeftDO:TDrawObjectPoint{In};Ward:integer{In}; var Height:integer{Out}
    ):TDrawObjectPoint{LastDrawObject Out};
var
  Height1:integer;
  curWard:integer;
  SecondLastDO,LeftDownDO,LastDownDO:TDrawObjectPoint;
begin
  LeftDownDO:=ALeftDO;
  result:=FirstOperand.DrawObjectsToDownWithBothDownPoints(
              AList,LeftDownDO,Ward,Height1,LastDownDO);
  inc(Height1,cVerticalSpace);
// ???
  if (ward=cwBACKWARD) and (ALeftDO=LeftDownDO) then
    curWard:=cwBACKWARD
  else
    curWard:=cwNONE;
  ALeftDO:=CreatePointToDown(AList, LeftDownDO, curWard, ALeftDO.y+height1,Owner.Owner);
  SecondLastDO:=SecondOperand.DrawObjectsToDown
          (AList,ALeftDO,Ward,Height);
// ???
  if (result=LastDownDO) and (ward=cwFORWARD) then
    curWard:=cwFORWARD
  else
    curWard:=cwNONE;
  LastDownDO.SecondInArrow:=TArrow.make(curWard, SecondLastDO);

  If SecondLastDO.x<=result.x then
    SecondLastDO.x:=result.x
  else
    result.x:=SecondLastDO.x;
  inc(Height,Height1);
end;

function TRE_Or.DrawObjectsToDownWithBothDownPoints(
     AList:TDrawObjectList{In Changed};var ALeftDO:TDrawObjectPoint{In};
     Ward:integer{In};var Height:integer{Out};
     var ALastDownDO:TDrawObjectPoint
     ):TDrawObjectPoint{LastDrawObject Out};
var
  Height1:integer;
  curWard:integer;
  SecondLastDO,LeftDownDO,LastDownDO:TDrawObjectPoint;
begin
  LeftDownDO:=ALeftDO;
  result:=FirstOperand.DrawObjectsToDownWithBothDownPoints(
              AList,LeftDownDO,Ward,Height1,LastDownDO);
  inc(Height1,cVerticalSpace);
// ???
  if (ward=cwBACKWARD) and (ALeftDO=LeftDownDO) then
    curWard:=cwBACKWARD
  else
    curWard:=cwNONE;
  ALeftDO:=CreatePointToDown(AList, LeftDownDO, curWard, ALeftDO.y+height1,Owner.Owner);
  SecondLastDO:=SecondOperand.DrawObjectsToDownWithBothDownPoints
          (AList,ALeftDO,Ward,Height,ALastDownDO);
// ???
  if (result=LastDownDO) and (ward=cwFORWARD) then
    curWard:=cwFORWARD
  else
    curWard:=cwNONE;
  LastDownDO.SecondInArrow:=TArrow.make(curWard, SecondLastDO);

  If SecondLastDO.x<=result.x then
    SecondLastDO.x:=result.x
  else
    result.x:=SecondLastDO.x;
  inc(Height,Height1);
end;

procedure TRE_BinaryOperation.unmarkAll;
begin
  Inherited unmarkAll;
  m_First.unmarkAll;
  m_Second.unmarkAll;
end;

function TRE_BinaryOperation.DrawObjectsCanDown(AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{Out};AFromDO:TDrawObjectPoint{In Out};
     Ward:integer{In}; var Height:integer{Out}
     ):TDrawObject{LastDrawObject Out};
begin
  ASemantic:=nil;
  result:=DrawObjectsToDown(AList,AFromDO,Ward,Height);
end;
function TRE_Iteration.DrawObjectsFromPoint(AList:TDrawObjectList{In Changed};
     var ASemantic:TSemanticIDList{Out};var AFromDO:TDrawObjectPoint{In Out};
     Ward:integer{In}; var Height:integer{Out}
     ):TDrawObject{LastDrawObject Out};
begin
  ASemantic:=nil;
  result:=DrawObjectsToRight(AList,ASemantic,AFromDO,Ward,Height);
end;
function TRE_BinaryOperation.DrawObjectsToPoint(AList:TDrawObjectList{In Changed};
     ASemantic:TSemanticIDList{In};AFromDO:TDrawObject{In};Ward:integer{In};
     var Height:integer{Out}
     ):TDrawObjectPoint{LastDrawObject Out};
begin
  result:=TDrawObjectPoint(DrawObjectsToRight(AList,ASemantic,AFromDO,Ward,Height));
end;

function TRE_Iteration.DrawObjectsToDown(AList:TDrawObjectList{In Changed};
    ALeftDO:TDrawObjectPoint{In};Ward:integer{In}; var Height:integer{Out}
    ):TDrawObjectPoint{LastDrawObject Out};
var
  Height1:integer;
  curWard:integer;
  LeftDO,LastDO,SecondLastDO,LastDownDO:TDrawObjectPoint;
begin
  LastDO:=FirstOperand.DrawObjectsToBothLastPoints(
              AList,nil,ALeftDO,Ward,Height1,LastDownDO);
  inc(Height1,cVerticalSpace);
// ???
  if ward=cwFORWARD then
    curWard:=cwBACKWARD
  else
    curWard:=cwNONE;
  LeftDO:=CreatePointToDown(AList, ALeftDO, curWard, ALeftDO.y+height1,Owner.Owner);
  SecondLastDO:=SecondOperand.DrawObjectsToDown(AList,LeftDO,-Ward,Height);
// ???
  curWard:=cwNone;
  if LastDO=LastDownDO then begin
    result:=LastDO;
    if ward=cwBACKWARD then
      curWard:=cwFORWARD
    else
      curWard:=cwNONE;
  end else begin
    result:=CreatePointToRight(AList,LastDO,curWard,Owner.Owner);
    curWard:=cwNONE;
  end;
  result.SecondInArrow:=TArrow.make(curWard, SecondLastDO);

  If SecondLastDO.x<=result.x then
    SecondLastDO.x:=result.x
  else
    result.x:=SecondLastDO.x;
  inc(Height,Height1);
end;

procedure TGrammar.OpenAllMacro;
begin
  m_MacroList.SetAllNotMarked;
  m_NonTerminalList.SetAllNotMarked;
  try
    m_MacroList.OpenAllMacro;
    m_NonTerminalList.OpenAllMacro;
  except
    on EAbort do CloseAllDefinition;
  end;
end;

procedure TGrammar.CloseAllDefinition;
begin
  m_MacroList.CloseAllDefinition;
  m_NonTerminalList.CloseAllDefinition;
end;

procedure TMacroList.SetAllNotMarked;
var
  i:integer;
begin
  for i:=0 to m_Items.Count-1 do
    TNTListItem(m_Items.Objects[i]).m_Mark:=0;
end;

function TNTListItem.setMarkAsOpenAllMacro:integer;
begin
  m_Mark:=cmInProgress;
  if m_Root.AllMacroWasOpened then begin
    m_Mark:=cmWasOpened;
  end else
    m_Mark:=cmWasNotOpened;
  result:=m_Mark;
end;

procedure TMacroList.OpenAllMacro;
var
  i:integer;
begin
  for i:=0 to m_Items.Count-1 do
    with m_Items.Objects[i] as TNTListItem do
      if Mark=cmNotMarked then
        setMarkAsOpenAllMacro;
  for i:=0 to m_Items.Count-1 do
    with m_Items.Objects[i] as TNTListItem do
      if Mark=cmWasOpened then setValueFromRoot;
end;

procedure TMacroList.CloseAllDefinition;
var
  i:integer;
begin
  for i:=0 to m_Items.Count-1 do
    with m_Items.Objects[i] as TNTListItem do
      if m_Root.AllDefinitionWasClosed then
        setValueFromRoot;
end;
function TRE_Tree.AllMacroWasOpened:boolean;
begin
  result:=false;
end;
function TRE_Tree.AllDefinitionWasClosed:boolean;
begin
  result:=false;
end;
function TRE_Macro.AllMacroWasOpened:boolean;
begin
  with ListItem do begin
    if Mark=cmNotMarked then
      setMarkAsOpenAllMacro;
    if Mark=cmInProgress then begin
      ERROR(Format(CantOpenAllMacro,[NameID]));
      Abort;
    end;
    Opened:=true;
    result:=true;
  end;
end;
function TRE_NonTerminal.AllMacroWasOpened:boolean;
begin
  if Opened then begin
    with ListItem do begin
      if Mark=cmNotMarked then
        setMarkAsOpenAllMacro;
      result:=(Mark=cmWasOpened);
    end;
  end
  else
    result:=false;
end;
function TRE_Macro.AllDefinitionWasClosed:boolean;
begin
  if opened then begin
    opened:=false;
    result:=true;
  end else
    result:=false;
end;
function TRE_BinaryOperation.AllMacroWasOpened:boolean;
begin
{$B+}
  result:=FirstOperand.AllMacroWasOpened or
            SecondOperand.AllMacroWasOpened;
{$B-}
end;

function TRE_BinaryOperation.AllDefinitionWasClosed:boolean;
begin
{$B+}
  result:=FirstOperand.AllDefinitionWasClosed or
            SecondOperand.AllDefinitionWasClosed;
{$B-}
end;
function TRE_Macro.getRoot:TRE_Tree;
begin
  result:=TGrammar(Owner).MacroObjects[m_ID].m_Root;
end;
function TRE_Macro.getListItem:TNTListItem;
begin
  result:=TGrammar(Owner).MacroObjects[m_ID];
end;
function TRE_NonTerminal.getListItem:TNTListItem;
begin
  result:=TGrammar(Owner).NonTerminalObjects[m_ID];
end;

function TRE_NonTerminal.getRoot:TRE_Tree;
begin
  result:=TGrammar(Owner).NonTerminalObjects[m_ID].m_Root;
end;
function TGrammar.getTerminals: TStrings;
begin
  result:=m_TerminalList.m_Items;
end;
function TGrammar.getSemantics: TStrings;
begin
  result:=m_SemanticList.m_Items;
end;
function TGrammar.getNonTerminals: TStrings;
begin
  result:=m_NonTerminalList.m_Items;
end;
function TGrammar.RemoveNonterminal(id: integer): integer;
begin
  result:=m_NonTerminalList.Remove(id);
end;
function TGrammar.getMacros: TStrings;
begin
  result:=m_MacroList.m_Items;
end;

Procedure TRE_Leaf.Save;
begin
  writeln(m_Op);
  writeln(m_ID);
end;
Procedure TRE_Macro.Save;
begin
  inherited;
  if opened then
    writeln('T')
  else
    writeln('F');
end;
Procedure TRE_BinaryOperation.Save;
begin
  writeln(m_Op);
  FirstOperand.Save;
  SecondOperand.Save;
end;
function LoadRE_Tree(Grammar:TComponent):TRE_Tree;
var
  OpType:integer;
  ID:integer;
  opened:char;
begin
  readln(OpType);
  case OpType of
    OperationTerminal:begin
      readln(ID);
      result:=TRE_Terminal.makeFromID(Grammar,ID);
    end;
    OperationSemantic:begin
      readln(ID);
      result:=TRE_Semantic.makeFromID(Grammar,ID);
    end;
    OperationNonTerminal:begin
      readln(ID);
      readln(opened);
      result:=TRE_NonTerminal.makeFromIDAndOpen(Grammar,ID,(Opened='T'));
    end;
    OperationMacro:begin
      readln(ID);
      readln(opened);
      result:=TRE_Macro.makeFromIDAndOpen(Grammar,ID,(Opened='T'));
    end;
    OperationOr:begin
      result:=LoadRE_Tree(Grammar);
      result:=TRE_Or.make(Grammar,result,LoadRE_Tree(Grammar));
    end;
    OperationAnd:begin
      result:=LoadRE_Tree(Grammar);
      result:=TRE_And.make(Grammar,result,LoadRE_Tree(Grammar));
    end;
    OperationIteration:begin
      result:=LoadRE_Tree(Grammar);
      result:=TRE_Iteration.make(Grammar,result,LoadRE_Tree(Grammar));
    end;
    else
      result:=nil
  end;
end;

Procedure TNTListItem.Save;
begin
  m_Root.save;
  m_Draw.Save;
end;

Procedure TNTListItem.Load;
begin
  m_Root:=LoadRE_Tree(Owner);
  m_Draw.Load;
end;

Procedure TTerminalList.LoadNames;
var
  count,i:integer;
  curStr:string;
begin
  readln(Count);
  for i:=0 to Count-1 do begin
    readln(curStr);
    add(curStr);
  end;
end;
Procedure TTerminalList.SaveNames;
var i:integer;
begin
  Writeln(m_Items.Count);
  for i:=0 to m_Items.Count-1 do
    Writeln(m_Items[i]);
end;
Procedure TMacroList.SaveObjects;
var
  i:integer;
  cnt:integer;
begin
  cnt:=m_Items.Count;
  writeln(Cnt);
  for i:=0 to cnt-1 do
    TNTListItem(m_Items.Objects[i]).Save;
end;

Procedure TMacroList.LoadObjects;
var
  count,i:integer;
begin
  readln(Count);
  for i:=0 to Count-1 do
    TNTListItem(m_Items.Objects[i]).Load;
  for i:=0 to Count-1 do
    setStringFromRoot(i);
end;
procedure TGrammar.ClearAllDictionaries;
begin
    m_TerminalList.m_Items.Clear;
    m_SemanticList.m_Items.Clear;
    m_MacroList.m_Items.Clear;
    m_NonTerminalList.m_Items.Clear;
end;
procedure TGrammar.Load(const FileName:string);
begin
  AssignFile(InPut, FileName);
  try
    ReSet(InPut);
    m_TerminalList.LoadNames;
    m_SemanticList.LoadNames;
    m_MacroList.LoadNames;
    m_NonTerminalList.LoadNames;
    m_MacroList.LoadObjects;
    m_NonTerminalList.LoadObjects;
    closeFile(InPut);
  except
    on EInOutError do begin
      closeFile(Input);
      MessageDlg('File I/O error.', mtError, [mbOk], 0);
    end;
    on Exception do
      closeFile(Input);
  end;
end;

// only for Import
  function BeginsWithEnvironment(const s:string):boolean;
  var
    i,len:integer;
  begin
    len:=length(s);
    if len>=lenEnvironment then
    begin
      result:=true;
      for i:=1 to lenEnvironment do
        if (UpCase(s[i])<>('ENVIRONMENT'[i])) then begin
          result:=false;
          exit;
        end;
    end else
      result:=false;
  end;
  function EndsWithNonTerminals(const s:string):boolean;
  var
    i,len:integer;
  begin
    len:=length(s);
    if len>=lenNonTerminals then
    begin
      result:=true;
      for i:=0 to lenNonTerminals-1 do
        if UpCase(s[len-i])<>'NONTERMINALS'[lenNonTerminals-i] then begin
          result:=false;
          break;
        end;
    end else
      result:=false;
  end;
  procedure TGrammar.AddToDictionary(DictionaryID:integer;ACharProducer:TCharProducer);
  var s:string;
    curChar:char;
  begin
    if DictionaryID=0 then
      repeat
        ACharProducer.next;//skip ':', ','
        SkipSpaces(ACharProducer);
        curChar:=ACharProducer.CurrentChar;
        ACharProducer.next;//skip '''', '"'
        s:=readName(ACharProducer,curChar);
        ACharProducer.next;//skip '''', '"'
        SkipSpaces(ACharProducer);
        addTerminalName(s);
      until ACharProducer.CurrentChar='.'
    else
      repeat
        ACharProducer.next;//skip ':', ','
        s:=readIdentifier(ACharProducer);
        if ACharProducer.CurrentChar='(' then begin
          SkipToChar(ACharProducer,')');
          ACharProducer.next;//skip ')'
          SkipSpaces(ACharProducer);
        end;
        addName(DictionaryID,s);
      until ACharProducer.CurrentChar='.';
  end;

procedure TGrammar.ImportFromRichEdit(RichEdit:TRichEdit);
var
  CharProducer:TCharProducer;
  str:string;
  DictionaryID:integer;
  f:integer;
begin
  ClearAllDictionaries;
  m_TerminalList.add('');
  CharProducer:=TStringCharProducer.make(RichEdit.Text);
  str:=readIdentifier(CharProducer);
  while not EndsWithNonTerminals(str) do begin
    CharProducer.next;
    str:=readIdentifier(CharProducer);
  end;
  DictionaryID:=cgNonTerminalList;
  while(not BeginsWithEnvironment(str))do begin
    if DictionaryID>=0 then begin
      AddToDictionary(DictionaryID,CharProducer);
    end else begin
      CharProducer.next;//skip ':'
      f:=FindMacroName(str);
      if f<0 then begin
        f:=FindNonTerminalName(str);
        if f>=0 then begin
          NonTerminalObjects[f].m_Root:=RE_FromCharProducer(CharProducer,Self);
        end else ERROR('Can''t find '+str+' in Macro and NonTerminals');
      end else begin
        MacroObjects[f].m_Root:=RE_FromCharProducer(CharProducer,Self);
      end;
    end;
    if not(CharProducer.next) then break;//skip '.'
    str:=readIdentifier(CharProducer);
    DictionaryID:=FindDictionaryName(str);
  end;
  setValuesFromRoot;
end;
procedure TGrammar.ImportFromSyntax(const FileName:string);
var
  CharProducer:TFileCharProducer;
  str:string;
  DictionaryID:integer;
  f:integer;
begin
  ClearAllDictionaries;
  m_TerminalList.add('');
  CharProducer:=TFileCharProducer.make(FileName);
  str:=readIdentifier(CharProducer);
  while not EndsWithNonTerminals(str) do begin
    if not CharProducer.next then
        raise Exception.Create('End of file passed');           // BUGFIX: die while wrong file format open   
    str:=readIdentifier(CharProducer);
  end;
  DictionaryID:=cgNonTerminalList;
  while(not BeginsWithEnvironment(str))do begin
    if DictionaryID>=0 then begin
      AddToDictionary(DictionaryID,CharProducer);
    end else begin
      if not(CharProducer.next) then break;//skip ':'
      f:=FindMacroName(str);
      if f<0 then begin
        f:=FindNonTerminalName(str);
        if f>=0 then begin
          NonTerminalObjects[f].m_Root:=RE_FromCharProducer(CharProducer,Self);
        end else ERROR('Can''t find '+str+' in Macro and NonTerminals');
      end else begin
        MacroObjects[f].m_Root:=RE_FromCharProducer(CharProducer,Self);
      end;
    end;
    if not(CharProducer.next) then break;//skip '.'
    str:=readIdentifier(CharProducer);
    DictionaryID:=FindDictionaryName(str);
  end;
  setValuesFromRoot;
  CharProducer.destroy;
end;
procedure TGrammar.ImportFromGEdit(const FileName:string);
var
  CharProducer:TFileCharProducer;
  str:string;
  f:integer;
begin
  ClearAllDictionaries;
  m_TerminalList.add('');
  CharProducer:=TFileCharProducer.make(FileName);
  str:={UpperCase}(readIdentifier(CharProducer));
  while true do begin
    if str='EOGRAM' then break;
    f:=findNonTerminalName(str);
    if f=-1 then f:=addNonTerminalName(str);
    if not(CharProducer.next) then break;//skip ':'
    NonTerminalObjects[f].m_Root:=RE_FromCharProducer2(CharProducer,Self);
    if not(CharProducer.next) then break;//skip '.'
    str:={UpperCase}(readIdentifier(CharProducer));
  end;
  setValuesFromRoot;
  CharProducer.destroy;
end;

procedure TGrammar.setValuesFromRoot;
begin
  m_MacroList.setAllValuesFromRoot;
  m_NonTerminalList.setAllValuesFromRoot;
end;
procedure TGrammar.Save(const FileName:string);
begin
  AssignFile(OutPut, FileName);
  try
    ReWrite(OutPut);
    m_TerminalList.SaveNames;
    m_SemanticList.SaveNames;
    m_MacroList.SaveNames;
    m_NonTerminalList.SaveNames;
    m_MacroList.SaveObjects;
    m_NonTerminalList.SaveObjects;
    CloseFile(OutPut);
  except
    on EInOutError do begin
      CloseFile(OutPut);
      MessageDlg('File I/O error.', mtError, [mbOk], 0);
    end;
  end;
end;
function TNTListItem.CopyRE_Tree:TRE_Tree;
begin
  result:=m_Root.Copy;
end;
procedure TNTListItem.setHeight(H:integer);
begin
  m_Draw.height:=H;
end;
procedure TNTListItem.setWidth(W:integer);
begin
  m_Draw.Width:=W;
end;

function TNTListItem.getHeight:integer;
begin
  result:=m_Draw.height;
end;

function TNTListItem.getWidth:integer;
begin
  result:=m_Draw.Width;
end;
Procedure TNTListItem.ChangeSelectionInRect(ARect:TRect);
begin
  m_Draw.ChangeSelectionInRect(ARect);
end;
function TNTListItem.FindDO(x,y:integer):TDrawObject;
begin
  result:=m_Draw.FindDO(x,y);
end;
Procedure TNTListItem.SelectedMoveTo(AX,AY:integer);
var
  dx,dy:integer;
begin
  dx:=AX-m_LastX;
  dy:=AY-m_LastY;
  m_Draw.SelectedMove(dx,dy);
  SelectedSetTo(AX,AY);
end;
Procedure TNTListItem.SelectedSetTo(AX,AY:integer);
begin
  m_LastX:=AX;
  m_LastY:=AY;
end;
Procedure TNTListItem.addExtendedPoint;
begin
  m_Draw.addExtendedPoint;
end;
Procedure TNTListItem.UnSelectAll;
begin
  m_Draw.UnSelectAll;
end;
Procedure TNTListItem.SelectAllNotSelected(ATarget:TDrawObject);
begin
  m_Draw.SelectAllNotSelected(ATarget);
end;
Procedure TNTListItem.ChangeSelection(ATarget:TDrawObject);
begin
  m_Draw.ChangeSelection(ATarget);
end;

procedure TNTListItem.Draw(Canvas:TCanvas);
begin
  m_Draw.Draw(Canvas);
end;

constructor TNTListItem.Create(Owner:TComponent);
begin
  inherited;
  m_Root:=nil;
  m_Draw:=TDrawObjectList.make(Owner);
end;

function TRE_BinaryOperation.getOperationChar:char;
begin
  case m_Op of
    OperationOr:result:=';';
    OperationAnd:result:=',';
    OperationIteration:result:='#';
    else result:='?';
  end;
end;

function TRE_Macro.getPriority:integer;
begin
  if m_Opened then
    result:=Root.getPriority
  else
    result:=0;
end;
function TRE_Tree.getPriority:integer;
begin
  case m_Op of
    OperationOr:result:=4;
    OperationAnd:result:=3;
    OperationIteration:result:=2;
  else
    result:=0;
  end;
end;

function TRE_Tree.Left: TRE_Tree;
begin
        result := nil;
end;

function TRE_Tree.Right: TRE_Tree;
begin
        result := nil;
end;

constructor TTerminalList.make(Owner:TComponent;Items:TStrings);
begin
  inherited create(Owner);
  m_Items:=Items;
end;
procedure TSemanticList.fillNew;
begin
  m_Items.clear;
end;
procedure TMacroList.fillNew;
begin
  m_Items.clear;
end;
procedure TTerminalList.fillNew;
begin
  m_Items.clear;
  add('');
end;

procedure TNonTerminalList.unmarkAllNonterminals;
var
  i:integer;
  curObject:TNTListItem;
begin
  for i:=1 to m_Items.Count do
  begin
    curObject := TNTListItem(m_Items.Objects[i]);
    curObject.m_Mark := cmNotMarked;
    curObject.m_Root.unmarkAll;
  end;
end;
procedure TNonTerminalList.markEmptyNonterminals;
var
  changed:boolean;
  i:integer;
  curObject:TNTListItem;
  mark:integer;
begin
  unMarkAllNonterminals;
  changed:=true;
  while changed do
  begin
    changed := false;
    for i:=1 to m_Items.Count do
    begin
      curObject := TNTListItem(m_Items.Objects[i]);
      if (curObject.m_Mark)=cmNotMarked then
      begin
        mark := curObject.m_Root.getEmptyMark;
        curObject.m_Mark := mark;
        if mark=cmMarkedEmpty then
        begin
          changed := true;
        end;
      end;
    end;
  end;
end;

procedure TNonTerminalList.regularizeGrammar;
var
  i,j:integer;
  curObject:TNTListItem;
begin
  markEmptyNonterminals();
  //todo:
  for i:=m_Items.Count downto 1 do
  begin
    curObject := TNTListItem(m_Items.Objects[i]);
    curObject.root.substituteAllEmpty;
    for j:=1 to i-1 do
    begin
{ todo:   makeFromID(curObject, i);
    if (curObject.m_Mark)=cmNotMarked then
    begin
}
    end;
  end;
end;

procedure TRE_BinaryOperation.substituteAllEmpty;
begin
    FirstOperand.substituteAllEmpty;
    SecondOperand.substituteAllEmpty;
end;

{todo:

procedure TRE_BinaryOperation.substitute(source, dest:TRE_Tree);
begin
  if (LeftOperand.equals(source)) then
  begin
    LeftOperand=dest;
  end else
  begin
    LeftOperand.substitute(source, dest);
  end;
  if (RightOperand.equals(source)) then
  begin
    RightOperand=dest;
  end else
  begin
    RightOperand.substitute(source, dest);
  end;
end;
}

procedure TRE_Leaf.substituteAllEmpty;
begin
end;

procedure TNonTerminalList.fillNew;
begin
  m_Items.clear;
  add('S');
end;
function TTerminalList.getLengthFromID(index:integer):integer;
begin
  result:=NormalSizeCanvas.TextWidth(m_Items[index]);
end;
function TTerminalList.getString(index:integer):string;
begin
  result:=m_Items[index];
end;
Function TRE_Terminal.copy:TRE_Tree;
begin
  result:=TRE_Terminal.makeFromID(Owner,m_ID);
end;
Function TRE_Macro.copy:TRE_Tree;
begin
  result:=TRE_Macro.makeFromID(Owner,m_ID);
end;
Function TRE_NonTerminal.copy:TRE_Tree;
begin
  result:=TRE_NonTerminal.makeFromID(Owner,m_ID);
end;
Function TRE_Semantic.copy:TRE_Tree;
begin
  result:=TRE_Semantic.makeFromID(Owner,m_ID);
end;

Function TRE_Or.copy:TRE_Tree;
begin
  result:=TRE_Or.make(Owner, m_First.copy, m_Second.copy);
end;
Function TRE_And.copy:TRE_Tree;
begin
  result:=TRE_And.make(Owner, m_First.copy, m_Second.copy);
end;
Function TRE_Iteration.copy:TRE_Tree;
begin
  result:=TRE_Iteration.make(Owner, m_First.copy, m_Second.copy);
end;
// Note for right elimination (RE):
// Parameter 'Reverse' needed to rearrange arguments of some operations
// in order to solve right recursion elimination task using left recursion
// elimination alorithm
// Only arguments of 'And' operation must be rearranged, other must not
// This method will be overrided only in 'TRE_And' class
function TRE_BinaryOperation.toString(SelMas: TSelMas; Reverse: boolean):string;    // Parameter for substitute, right elim
var
  p:integer;
begin
  p:=Priority;
  if FirstOperand.Priority<=p then
    result:=FirstOperand.toString(SelMas, Reverse)                    // Parameter for substitute, RE
  else
    result:='('+FirstOperand.toString(SelMas, Reverse)+')';           // Parameter for substitute, RE
  result:=result+OperationChar;
  if SecondOperand.Priority<p then
    result:=result+SecondOperand.toString(SelMas, Reverse)            // Parameter for substitute, RE
  else
    result:=result+'('+SecondOperand.toString(SelMas, Reverse)+')';   // Parameter for substitute, RE
end;

function TRE_And.toString(SelMas: TSelMas; Reverse: boolean):string;    // Parameter for substitute, right elim
var
  p:integer;
  first_result, second_result: string;
begin
  p:=Priority;
  if FirstOperand.Priority<=p then
    first_result:=FirstOperand.toString(SelMas, Reverse)              // RE
  else
    first_result:='('+FirstOperand.toString(SelMas, Reverse)+')';     // RE
//  result:=result+OperationChar;
  if SecondOperand.Priority<p then
    second_result:=SecondOperand.toString(SelMas, Reverse)            // RE
  else
    second_result:='('+SecondOperand.toString(SelMas, Reverse)+')';   // RE
  if not Reverse then                                                 // Reverse if needed
    result := first_result + OperationChar + second_result
  else
    result := second_result + OperationChar + first_result;
end;

function TRE_BinaryOperation.Left: TRE_Tree;
begin
   result := FirstOperand;
end;

function TRE_BinaryOperation.Right: TRE_Tree;
begin
   result := SecondOperand; 
end;

function TRE_Leaf.toString(SelMas: TSelMas; Reverse: boolean):string;          // Parameter for substitute, RE
begin
  result:=getNameFromID;
end;

function TRE_Leaf.Left: TRE_Tree;  // left child
begin
  result:=nil;
end;

function TRE_Leaf.Right: TRE_Tree;   // right child
begin
  result:=nil;
end;

function TRE_Terminal.toString(SelMas: TSelMas; Reverse: boolean):string;      // Parameter for substitute, RE
var
  res:string;
  Index:integer;
begin
  res:=getNameFromID;
  index:=Pos('''',res);
  if index=0 then
    result:=''''+res+''''
  else result:='"'+res+'"';
end;

function TRE_Terminal.Left: TRE_Tree;   // left child
begin
        result:=nil;
end;

function TRE_Terminal.Right: TRE_Tree;  // right child
begin
        result:=nil;
end;
//TODO: Test it - RE and reversing
function TRE_Macro.toString(SelMas: TSelMas; Reverse: boolean):string;      // Parameter for substitute, RE
var i: integer;            // for substitute
    substitute: boolean;   // for substitute
begin
  if opened then
    result:=root.toString(SelMas, Reverse)      // Parameter for substitute
  else
    result:=getNameFromID;

  if Self is TRE_NonTerminal then
     begin
     substitute:=false;
     for i:=0 to length(SelMas)-1 do
         if DrawObj=SelMas[i] then substitute:=true;
     if substitute then
        for i:=0 to (TChildForm(MainForm.ActiveMDIChild)).ActiveListBox.Items.Count-1 do
            if (TChildForm(MainForm.ActiveMDIChild)).ActiveListBox.Items[i]=result
               then begin
                    result:=TNTListItem((TChildForm(MainForm.ActiveMDIChild)).ActiveListBox.Items.Objects[i]).Value;
                    if result[length(result)]='.' then result:=system.copy(result,1,length(result)-1);
                    result:='('+result+')';
                    end;
     end;   // Change NonTerminal_name if selected. For substitute
end;

function TRE_Leaf.getNameFromID:string;
begin
  with Owner as TGrammar do
  begin
    result:=GetName(m_Op,m_Id)
  end;
end;

procedure TRE_Leaf.setNameFromID(const name:string);
begin
  with Owner as TGrammar do
  begin
    m_ID:=Find(m_Op,Name)
  end;
end;

function TTerminalList.add(const s:string):integer;
var
  f:integer;
begin
  f:=Find(s);
  if f<0 then f:=m_Items.Add(s);
  result:=f;
end;

function TSemanticList.add(const s:string):integer;
begin
  result:=m_Items.indexOf(s);
  if result=-1 then result:=m_Items.Add(s);
end;
function TMacroList.add(const s:string):integer;
var f:integer;
begin
  f:=-1;
  if not(TGrammar(Owner).FindName(s)) then begin
    f:=m_Items.Add(s);
    m_Items.Objects[f]:=TNTListItem.Create(Owner);
    setValue(f,'&.');
  end;
  result:=f;
end;

function TTerminalList.Find(const s:string):integer;
var
  f,i:integer;
begin
  f:=-1;
  for i:=0 to m_Items.count-1 do
    if s=m_Items[i] then begin
      f:=i;
      break;
    end;
  result:=f;
end;

procedure TMacroList.setValue(index:integer; const pValue:string);
begin
  TNTListItem(m_Items.Objects[index]).setValue(pValue)
end;
procedure TMacroList.setAllValuesFromRoot;
var i:integer;
begin
  for i:=0 to m_Items.count-1 do
    setValueFromRoot(i);
end;
procedure TMacroList.setValueFromRoot(index:integer);
begin
  TNTListItem(m_Items.Objects[index]).setValueFromRoot;
end;
procedure TMacroList.setStringFromRoot(index:integer);
begin
  TNTListItem(m_Items.Objects[index]).setStringFromRoot;
end;
procedure TNTListItem.setStringFromRoot;
var a: TSelMas;                              // For substitute
begin
  SetLength(a,0);                            // For substitute
  m_Value:=m_Root.toString(a, false)+'.';     // Parameter for substitute
end;
procedure TNTListItem.setValueFromRoot;
begin
  setStringFromRoot;
  CreateDrawObjects(m_Draw, m_Root, Owner.Owner);
end;
procedure TNTListItem.setRootFromValue;
begin
  if m_Root<>nil then m_Root.free;
  m_Root:=RE_FromCharProducer(TStringCharProducer.make(m_Value), TGrammar(Owner));
end;

procedure TNTListItem.setValue(const pValue:string);
begin
  m_Root:=RE_FromCharProducer(TStringCharProducer.make(pValue), TGrammar(Owner));
  setValueFromRoot;
  CreateDrawObjects(m_Draw, m_Root, Owner.Owner);
end;

function TMacroList.getValue(index:integer):string;
begin
  with m_Items.Objects[index] as TNTListItem do
    result:=m_Value;
end;

Constructor TGrammar.make(Owner:TComponent;TerminalList,
      SemanticList,NonTerminalList,MacroList:TStrings);
begin
  inherited Create(Owner);
  m_TerminalList:= TTerminalList.make(Self,TerminalList);
  m_SemanticList := TSemanticList.make(Self,SemanticList);
  m_NonTerminalList := TNonTerminalList.make(Self,NonTerminalList);
  m_MacroList := TMacroList.make(Self,MacroList);
end;

procedure TGrammar.regularizeGrammar;
begin
  m_NonTerminalList.regularizeGrammar;
end;

procedure TGrammar.fillNew;
begin
  m_TerminalList.fillNew;
  m_SemanticList.fillNew;
  m_MacroList.fillNew;
  m_NonTerminalList.fillNew;
end;
function TGrammar.FindName(const Name:string):boolean;
begin
  result:=not
   ((FindNonTerminalName(Name)<0)
   and(FindSemanticName(Name)<0)
   and(FindMacroName(Name)<0));
end;
function TGrammar.FindTerminalName(const Name:string):integer;
begin
  result:=m_TerminalList.find(Name);
end;

function TGrammar.AddTerminalName(const Name:string):integer;
begin
  result:=m_TerminalList.add(Name);
end;
function TGrammar.AddNonTerminalName(const Name:string):integer;
begin
  result:=m_NonTerminalList.add(Name);
end;
function TGrammar.AddMacroName(const Name:string):integer;
begin
  result:=m_MacroList.add(Name);
end;
function TGrammar.AddSemanticName(const Name:string):integer;
begin
  result:=m_SemanticList.add(Name);
end;
function TGrammar.AddName(listID:integer;const Name:string):integer;
begin
   case listID of
     cgTerminalList: result:= AddTerminalName(Name);
     cgSemanticList: result:= AddSemanticName(Name);
     cgNonTerminalList: result:= AddNonTerminalName(Name);
     cgMacroList: result:= AddMacroName(Name);
   else result:=-1;
   end;
end;

function TGrammar.getFreeNonTerminalName:string;
var
  index:integer;
begin
  index:=1;
  result:='NT'+IntToStr(index);
  While FindName(result) do begin
    inc(index);
    result:='NT'+IntToStr(index);
  end;
end;
function TGrammar.getFreeMacroName:string;
var
  index:integer;
begin
  index:=1;
  result:='MACRO'+IntToStr(index);
  While FindName(result) do begin
    inc(index);
    result:='MACRO'+IntToStr(index);
  end;
end;
function TGrammar.getFreeSemanticName:string;
var
  index:integer;
begin
  index:=1;
  result:='S'+IntToStr(index);
  While FindName(result) do begin
    inc(index);
    result:='S'+IntToStr(index);
  end;
end;
function TGrammar.FindSemanticName(const Name:string):integer;
begin
  result:=m_SemanticList.Find(Name);
end;
function TGrammar.FindNonTerminalName(const Name:string):integer;
begin
  result:=m_NonTerminalList.Find(Name);
end;
function TGrammar.FindMacroName(const Name:string):integer;
begin
  result:=m_MacroList.Find(Name);
end;
function TGrammar.Find(listID:integer;const Name:string):integer;
begin
   case listID of
     cgTerminalList: result:= AddTerminalName(Name);
     cgSemanticList: result:= FindSemanticName(Name);
     cgNonTerminalList: result:= FindNonTerminalName(Name);
     cgMacroList: result:= FindMacroName(Name);
   else result:=-1;
   end;
end;
function TGrammar.MacroObject(id:integer):TNTListItem;
begin
  result:=TNTListItem(m_MacroList.m_Items.Objects[id]);
end;
function TGrammar.NonTerminalObject(id:integer):TNTListItem;
begin
  result:=TNTListItem(m_NonTerminalList.m_Items.Objects[id]);
end;
function TGrammar.getTerminalName(NameID:integer):string;
begin
  result:=m_TerminalList.m_Items[NameID];
end;
function TGrammar.getSemanticName(NameID:integer):string;
begin
  result:=m_SemanticList.m_Items[NameID];
end;
function TGrammar.getNonTerminalName(NameID:integer):string;
begin
  result:=m_NonTerminalList.m_Items[NameID];
end;
function TGrammar.getMacroName(NameID:integer):string;
begin
  result:=m_MacroList.m_Items[NameID];
end;
function TGrammar.GetName(ListID, NameID:integer):string;
begin
   case listID of
     cgTerminalList: result:= GetTerminalName(NameID);
     cgSemanticList: result:= GetSemanticName(NameID);
     cgNonTerminalList: result:= GetNonTerminalName(NameID);
     cgMacroList: result:= GetMacroName(NameID);
   end;
end;
constructor TRE_Terminal.Create(AOwner:TComponent);
begin
    InHerited;
    m_Op:=OperationTerminal;
    m_ID:=0;
end;
constructor TRE_Terminal.makeFromID(AOwner:TComponent; ID:integer);
begin
    InHerited Create(AOwner);
    m_Op:=OperationTerminal;
    m_ID:=ID;
end;
constructor TRE_Macro.Create(AOwner:TComponent);
begin
    InHerited;
    m_Op:=OperationMacro;
    m_ID:=0;
    m_Opened:=MainForm.MacroOpened;
end;

constructor TRE_Macro.makeFromID(AOwner:TComponent; ID:integer);
begin
    InHerited Create(AOwner);
    m_Op:=OperationMacro;
    m_ID:=ID;
    m_Opened:=MainForm.MacroOpened;
end;

constructor TRE_Macro.makeFromIDAndOpen(AOwner:TComponent; ID:integer; open:boolean);
begin
    InHerited Create(AOwner);
    m_Op:=OperationMacro;
    m_ID:=ID;
    m_Opened:=open;
end;

constructor TRE_Semantic.Create(AOwner:TComponent);
begin
    InHerited;
    m_Op:=OperationSemantic;
end;
constructor TRE_Semantic.makeFromID(AOwner:TComponent; ID:integer);
begin
    InHerited Create(AOwner);
    m_Op:=OperationSemantic;
    m_ID:=ID;
end;
constructor TRE_NonTerminal.Create(AOwner:TComponent);
begin
    InHerited ;
    m_Op:=OperationNonTerminal;
    m_Opened:=false;
end;
constructor TRE_NonTerminal.makeFromID(AOwner:TComponent; ID:integer);
begin
    InHerited Create(AOwner);
    m_Op:=OperationNonTerminal;
    m_ID:=ID;
    m_Opened:=false;
end;
constructor TRE_NonTerminal.makeFromIDAndOpen(AOwner:TComponent; ID:integer; open:boolean);
begin
    InHerited Create(AOwner);
    m_Op:=OperationNonTerminal;
    m_ID:=ID;
    m_Opened:=open;
end;
constructor TRE_Or.make(AOwner:TComponent; first,Second:TRE_Tree);
begin
    InHerited Create(AOwner);
    m_Op:=OperationOr;
    m_First:=First;
    m_Second:=Second;
end;
constructor TRE_And.make(AOwner:TComponent; first,Second:TRE_Tree);
begin
    InHerited Create(AOwner);
    m_Op:=OperationAnd;
    m_First:=First;
    m_Second:=Second;
end;

constructor TRE_Iteration.make(AOwner:TComponent; first,Second:TRE_Tree);
begin
    InHerited Create(AOwner);
    m_Op:=OperationIteration;
    m_First:=First;
    m_Second:=Second;
end;
function TTerminalList.remove(id: integer): integer;
begin
  m_Items.Delete(id);
  result:=id;
end;

end.
