import { requestGet, requestDel, requestPut, requestPost, setToken } from "./agent.bs.js";

const encode = encodeURIComponent;

const Auth = {
  current: () =>
  requestGet('/user'),
  login: (email, password) =>
    requestPost('/users/login', { user: { email, password } }),
  register: (username, email, password) =>
    requestPost('/users', { user: { username, email, password } }),
  save: user =>
    requestPut('/user', { user })
};

const Tags = {
  getAll: () => requestGet('/tags')
};

const limit = (count, p) => `limit=${count}&offset=${p ? p * count : 0}`;
const omitSlug = article => Object.assign({}, article, { slug: undefined })
const Articles = {
  all: page =>
    requestGet(`/articles?${limit(10, page)}`),
  byAuthor: (author, page) =>
    requestGet(`/articles?author=${encode(author)}&${limit(5, page)}`),
  byTag: (tag, page) =>
    requestGet(`/articles?tag=${encode(tag)}&${limit(10, page)}`),
  del: slug =>
    requestDel(`/articles/${slug}`),
  favorite: slug =>
    requestPost(`/articles/${slug}/favorite`),
  favoritedBy: (author, page) =>
    requestGet(`/articles?favorited=${encode(author)}&${limit(5, page)}`),
  feed: () =>
    requestGet('/articles/feed?limit=10&offset=0'),
  get: slug =>
    requestGet(`/articles/${slug}`),
  unfavorite: slug =>
    requestDel(`/articles/${slug}/favorite`),
  update: article =>
    requestPut(`/articles/${article.slug}`, { article: omitSlug(article) }),
  create: article =>
    requestPost('/articles', { article })
};

const Comments = {
  create: (slug, comment) =>
    requestPost(`/articles/${slug}/comments`, { comment }),
  delete: (slug, commentId) =>
    requestDel(`/articles/${slug}/comments/${commentId}`),
  forArticle: slug =>
    requestGet(`/articles/${slug}/comments`)
};

const Profile = {
  follow: username =>
    requestPost(`/profiles/${username}/follow`),
  get: username =>
    requestGet(`/profiles/${username}`),
  unfollow: username =>
    requestDel(`/profiles/${username}/follow`)
};

export default {
  Articles,
  Auth,
  Comments,
  Profile,
  Tags,
  setToken,
};
