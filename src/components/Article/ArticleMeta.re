let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~article, ~canModify, ~onClickDelete, _children) => {
  ...component,
  render: _self =>
    <div className="article-meta">
      <Link _to={"/@" ++ article##author##username}>
        <img src=article##author##image alt=article##author##username />
      </Link>
      <div className="info">
        <Link _to={"/@" ++ article##author##username} className="author">
          {ReasonReact.string(article##author##username)}
        </Link>
        <span className="date">
          {
            Js.Date.fromString(article##createdAt)
            ->Js.Date.toDateString
            ->ReasonReact.string
          }
        </span>
      </div>
      <ArticleActions canModify article onClickDelete />
    </div>,
};
