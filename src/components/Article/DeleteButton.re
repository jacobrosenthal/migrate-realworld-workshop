let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~onClick, ~show, ~slug, ~commentId, _children) => {
  let del = _evt => {
    let payload = Agent.Comments.delete(slug, commentId);
    onClick(payload, commentId);
  };

  {
    ...component,
    render: _self =>
      if (show) {
        <span className="mod-options">
          <i className="ion-trash-a" onClick=del />
        </span>;
      } else {
        ReasonReact.null;
      },
  };
};
