unit TransTerminal;
interface
uses
  classes, TransLeaf;

type
  TTerminal = class(TLeaf)
    function equals(aTerminal :TTerminal):boolean;
    function isEmpty:boolean;
  end;

implementation

function TTerminal.equals(aTerminal :TTerminal):boolean;
begin
  result:=(getName = aTerminal.getName);
end;

function TTerminal.isEmpty():boolean;
begin
  result:=(getName = '');
end;

end.

