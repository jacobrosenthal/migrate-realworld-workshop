let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~article, ~onClickDelete, ~canModify, _children) => {
  let del = _evt => onClickDelete(Agent.Articles.del(article##slug));

  {
    ...component,
    render: _self =>
      if (canModify) {
        <span>
          <Link
            _to={"/editor/" ++ article##slug}
            className="btn btn-outline-secondary btn-sm">
            <i className="ion-edit" />
            {ReasonReact.string("Edit Article")}
          </Link>
          <button className="btn btn-outline-danger btn-sm" onClick=del>
            <i className="ion-trash-a" />
            {ReasonReact.string("Delete Article")}
          </button>
        </span>;
      } else {
        <span />;
      },
  };
};
