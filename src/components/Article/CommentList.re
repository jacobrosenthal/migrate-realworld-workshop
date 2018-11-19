let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~comments, ~slug, ~currentUser=?, ~onDelete, _children) => {
  ...component,
  render: _self =>
    <div>
      ...{
           Belt.Array.map(comments, comment =>
             <Comment
               comment
               ?currentUser
               slug
               onDelete
               key={string_of_int(comment##id)}
             />
           )
         }
    </div>,
};
