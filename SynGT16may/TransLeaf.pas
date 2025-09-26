unit TransLeaf;

interface
uses
  classes;

type
  TLeaf = class(TComponent)
  protected
    { Protected declarations }
    m_Name:string;
  public
    { Public declarations }
    constructor Create(aOwner:TComponent; aName:string);virtual;
    function getName:string;virtual;
  end;

implementation

constructor TLeaf.Create(aOwner:TComponent; aName:string);
begin
  inherited Create(aOwner);
  m_Name:=aName;
end;

function TLeaf.getName():string;
begin
  result:=m_Name;
end;
end.
