unit TransNonterminal;

interface
uses
  classes, TransLeaf, TransRE_Tree;

type
  TNonterminal = class(TLeaf)
  private
    m_Tree:TRE_Tree;
    m_AuxilaryNotion:boolean;
  public
    procedure setTree(aTree:TRE_Tree);
    function getTree():TRE_Tree;
    function equals(aNonterminal :TNonterminal):boolean;
    procedure substituteAll(aNonterminal: TRE_Tree; newRoot: TRE_Tree);
    property tree:TRE_Tree read getTree write setTree;
    property AuxilaryNotion:boolean read m_AuxilaryNotion write m_AuxilaryNotion;
  end;

implementation

function TNonterminal.equals(aNonterminal :TNonterminal):boolean;
begin
  result:=(getName = aNonterminal.getName());
end;

function TNonterminal.getTree():TRE_Tree;
begin
  result:=m_Tree;
end;

procedure TNonterminal.setTree(aTree:TRE_Tree);
begin
  m_Tree.Free;
  m_Tree:=aTree;
end;

procedure TNonterminal.substituteAll(aNonterminal: TRE_Tree; newRoot: TRE_Tree);
begin
  if (m_Tree.equals(aNonterminal)) then begin
    m_Tree:=newRoot;
  end else begin
    m_Tree.substituteAll(aNonterminal, newRoot);
  end;
end;


end.
