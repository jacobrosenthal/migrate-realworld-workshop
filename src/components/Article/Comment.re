let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~comment, ~currentUser=?, ~slug, ~onDelete, _children) => {
  ...component,
  render: _self => {
    let show =
      switch (currentUser) {
      | Some(currentUser) =>
        currentUser##username === comment##author##username
      | None => false
      };

    <div className="card">
      <div className="card-block">
        <p className="card-text"> {ReasonReact.string(comment##body)} </p>
      </div>
      <div className="card-footer">
        <Link
          _to={"/@" ++ comment##author##username} className="comment-author">
          <img
            src=comment##author##image
            className="comment-author-img"
            alt=comment##author##username
          />
        </Link>
        {ReasonReact.string(" ")}
        <Link
          _to={"/@" ++ comment##author##username} className="comment-author">
          {ReasonReact.string(comment##author##username)}
        </Link>
        <span className="date-posted">
          {
            Js.Date.fromString(comment##createdAt)
            ->Js.Date.toDateString
            ->ReasonReact.string
          }
        </span>
        <DeleteButton show slug commentId=comment##id onClick=onDelete />
      </div>
    </div>;
  },
};
