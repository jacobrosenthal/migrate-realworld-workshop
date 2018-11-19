type author = {
  .
  "bio": Js.Nullable.t(string),
  "following": bool,
  "image": string,
  "username": string,
};

type comment = {
  .
  "author": author,
  "body": string,
  "createdAt": string,
  "id": int,
  "updatedAt": string,
};

type article = {
  .
  "author": author,
  "body": string,
  "createdAt": string,
  "description": string,
  "favorited": bool,
  "favoritesCount": int,
  "slug": string,
  "tagList": array(string),
  "title": string,
  "updatedAt": string,
};

type errors = Js.Dict.t(array(string));

type state = {
  article: option(article),
  comments: array(comment),
  errors: option(errors),
};

type action =
  | Loaded(article, array(comment))
  | CommentError(errors)
  | AddComment(comment)
  | DeleteComment(int);

let component = ReasonReact.reducerComponent(__MODULE__);
