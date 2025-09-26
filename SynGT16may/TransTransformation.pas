unit TransTransformation;

interface

function EllimLeft(grammardesc: ansistring; name: string): string; // for right elim
function EllimBoth(grammardesc: ansistring; name: string): string;
function EllimRight(grammardesc: ansistring; name: string): string;

function Minimize(grammardesc: ansistring; name: string): string;

implementation

uses TransGrammar, SysUtils;

function EllimLeft(grammardesc: ansistring; name: string): string; // for right elim
begin
  defaultGrammar.Free();
  defaultGrammar:=TGrammar.loadFromStr(nil, grammardesc);
  defaultGrammar.leftEl();
  Result:=defaultGrammar.printSubstituted(name)+'.';
end;

function EllimBoth(grammardesc: ansistring; name: string): string;
begin
  defaultGrammar.Free();
  defaultGrammar:=TGrammar.loadFromStr(nil, grammardesc);
  defaultGrammar.regularize();
  Result:=defaultGrammar.printSubstituted(name)+'.';
end;

function EllimRight(grammardesc: ansistring; name: string): string;
begin
  defaultGrammar.Free();
  defaultGrammar:=TGrammar.loadFromStr(nil, grammardesc);
  defaultGrammar.RightEl();
  Result:=defaultGrammar.printSubstituted(name)+'.';
end;

function Minimize(grammardesc: ansistring; name: string): string;
begin
  defaultGrammar.Free();
  defaultGrammar:=TGrammar.loadFromStr(nil, grammardesc);
  defaultGrammar.Minimize();
  Result:=defaultGrammar.printSubstituted(name)+'.';
end;

end.
