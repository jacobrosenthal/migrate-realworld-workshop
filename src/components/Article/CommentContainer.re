let component = ReasonReact.statelessComponent(__MODULE__);

let make =
    (
      ~currentUser=?,
      ~slug,
      ~comments,
      ~onSubmit,
      ~onDelete,
      ~errors=?,
      _children,
    ) => {
  ...component,
  render: _self =>
    switch (currentUser) {
    | Some(currentUser) =>
      <div className="col-xs-12 col-md-8 offset-md-2">
        <div>
          <ListErrors ?errors />
          <CommentInput slug currentUser onSubmit />
        </div>
        <CommentList comments slug currentUser onDelete />
      </div>
    | None =>
      <div className="col-xs-12 col-md-8 offset-md-2">
        <p>
          <Link _to="/login"> {ReasonReact.string("Sign in")} </Link>
          {ReasonReact.string(" or ")}
          <Link _to="/register"> {ReasonReact.string("sign up")} </Link>
          {ReasonReact.string(" to add comments on this article.")}
        </p>
        <CommentList comments slug ?currentUser onDelete />
      </div>
    },
};
