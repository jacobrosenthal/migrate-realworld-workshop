module Comment = {
  [@bs.module "./Comment.js"]
  external reactClass: ReasonReact.reactClass = "default";

  let make = (~comment, ~currentUser, ~slug, _children) =>
    ReasonReact.wrapJsForReason(
      ~reactClass,
      ~props={"comment": comment, "currentUser": currentUser, "slug": slug},
      [||],
    );
};

let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~comments, ~slug, ~currentUser, _children) => {
  ...component,
  render: _self =>
    <div>
      ...{
           Belt.Array.map(comments, comment =>
             <Comment comment currentUser slug key=comment##id />
           )
         }
    </div>,
};
