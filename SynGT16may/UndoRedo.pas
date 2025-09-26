unit UndoRedo;

interface

uses RegularExpression,          // Project units
     Classes, StdCtrls, Dialogs; // Delphi units

type PUndoState = ^TUndoState;
     TUndoState = record
                  nt_names: TStringList;
                  nt_values: TStringList;
                  ind: integer;                // Active nonterminal index
                  selection: TSelMas;          // Selection mask
                  prev: PUndoState;
                  next: PUndoState;
                  end;

     TUndoRedo = class (TComponent)
     public
       beg: PUndoState;  // begin of list
       cur: PUndoState;  // current state
       lst: PUndoState;  // end of list
       Constructor Create(Owner:TComponent);override;
       Function EqualState(state1,state2:PUndoState):boolean;
       Procedure AddState(ActiveListBox:TListBox; ind:integer; SelMas:TSelMas);
       Procedure StepBack(var Grammar:TGrammar; var ActiveListBox:TListBox; var ind:integer; var SelMas:TSelMas);
       Procedure StepForward(var Grammar:TGrammar; var ActiveListBox:TListBox; var ind:integer; var SelMas:TSelMas);
       Procedure ClearData;
     end;

implementation

Constructor TUndoRedo.Create(Owner:TComponent);
begin
inherited;
beg:=nil;
cur:=nil;
lst:=nil;
end;

Function TUndoRedo.EqualState(state1,state2:PUndoState):boolean;
var i: integer;
begin
result:=false;
if (state1<>nil) and (state2<>nil) then
   begin
   if (state1.nt_names.Count=state2.nt_names.Count) and
      (state1.nt_values.Count=state2.nt_values.Count) and
      (state1.nt_names.Count=state1.nt_values.Count) then
      begin
      result:=true;
      for i:=0 to state1.nt_names.Count-1 do
          if (state1.nt_names[i]<>state2.nt_names[i]) or
             (state1.nt_values[i]<>state2.nt_values[i]) then result:=false;
      end;  // if count=count
   end;  // if not nil
end;  // proc

Procedure TUndoRedo.AddState(ActiveListBox:TListBox; ind:integer; SelMas:TSelMas);
var new_cur,next: PUndoState;
    i: integer;
begin
try
  if cur=nil then next:=nil
             else next:=cur.next;
  new(new_cur);
  // Data:
  new_cur.nt_names:=TStringList.Create;
  new_cur.nt_values:=TStringList.Create;
  if (cur<>nil)and(length(SelMas)>0) then cur.selection:=SelMas;
  new_cur.ind:=ind;
  for i:=0 to ActiveListBox.Items.Count-1 do
      begin
      new_cur.nt_names.Append(ActiveListBox.Items[i]);
      new_cur.nt_values.Append(TNTListItem(ActiveListBox.Items.Objects[i]).Value);
      end;
  if EqualState(cur,new_cur) or EqualState(next,new_cur)
     then begin
          // Equal State!!! Drop this!!
          new_cur.nt_names.Free;
          new_cur.nt_values.Free;
          SetLength(new_cur.selection,0);
          dispose(new_cur);
          end
     else begin
          // Pointers:
          new_cur.prev:=cur;
          new_cur.next:=next;

          if beg=nil then beg:=new_cur;
          if new_cur.next=nil then lst:=new_cur;

          if cur<>nil then cur.next:=new_cur;
          if next<>nil then next.prev:=new_cur;
          // Move pointer:
          cur:=new_cur;
          end;
except
  MessageDlg('Not enough memory', mtError, [mbOk], 0);
end;  // try
end;  // proc

Procedure TUndoRedo.StepBack(var Grammar:TGrammar; var ActiveListBox:TListBox; var ind:integer; var SelMas:TSelMas);
var i: integer;
begin
if cur.prev<>nil then
   begin
//   Grammar.ClearAllDictionaries
   ActiveListBox.Clear;

   cur:=cur.prev;
   for i:=0 to cur.nt_names.Count-1 do
       begin
       Grammar.AddNonTerminalName(cur.nt_names[i]);
//       ActiveListBox.Items.Append(cur.nt_names[i]);
//       ActiveListBox.Items.Objects[i]:=TNTListItem.Create(Owner);

{       TNTListItem(ActiveListBox.Items.Objects[j]).setValue(cur.nt_values[i]);
       TNTListItem(ActiveListBox.Items.Objects[j]).setRootFromValue;}
       end;  // for
   for i:=0 to cur.nt_names.Count-1 do
       begin
       TNTListItem(ActiveListBox.Items.Objects[i]).setValue(cur.nt_values[i]);
       TNTListItem(ActiveListBox.Items.Objects[i]).setRootFromValue;
       end;  // for
   ind:=cur.ind;
   SelMas:=cur.selection;
   end;  // if not nil
end;  // proc

Procedure TUndoRedo.StepForward(var Grammar:TGrammar; var ActiveListBox:TListBox; var ind:integer; var SelMas:TSelMas);
var i: integer;
begin
if cur.next<>nil then
   begin
   ActiveListBox.Clear;

   cur:=cur.next;
   for i:=0 to cur.nt_names.Count-1 do
       begin
       Grammar.AddNonTerminalName(cur.nt_names[i]);
       end;  // for
   for i:=0 to cur.nt_names.Count-1 do
       begin
       TNTListItem(ActiveListBox.Items.Objects[i]).setValue(cur.nt_values[i]);
       TNTListItem(ActiveListBox.Items.Objects[i]).setRootFromValue;
       end;  // for
   ind:=cur.ind;
   SelMas:=cur.selection;
   end;  // if not nil
end;  // proc

Procedure TUndoRedo.ClearData;
begin
while beg<>nil do
  begin
  cur:=beg.next;
  beg.nt_names.Free;
  beg.nt_values.Free;
  SetLength(beg.selection,0);
  dispose(beg);
  beg:=cur;
  end;
end;

end.
